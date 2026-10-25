using System;
using System.Collections.Generic;
using System.Numerics;
using System.Reflection;

namespace Morava
{
    // The kinds of public fields the editor shows and the scene saves: ScriptFieldType's values (src/Scripting/ScriptField.h)
    enum FieldKind { Bool, Int, Float, Double, String, Vector2, Vector3, Vector4, Entity, Enum, Prefab }

    // A script's public fields for the engine (docs/engine/Game_Engine_Runtime_Guide.html, 4.5): which ones there are, with
    // the values a new instance has, and setting and reading them by name. Reflection, never cached: a cached Type or
    // FieldInfo would hold the game's context alive (hot reload, 4.6). It runs when the game loads, when a game starts and
    // for the one entity the Inspector shows, not per frame and script.
    static unsafe class ScriptFields
    {
        // A class the engine creates for entities
        internal static bool IsScript(Type type) =>
            type.IsClass && !type.IsAbstract && !type.ContainsGenericParameters && type.IsSubclassOf(typeof(Entity));

        internal static FieldKind? KindOf(Type type)
        {
            if (type == typeof(bool)) return FieldKind.Bool;
            if (type == typeof(int)) return FieldKind.Int;
            if (type == typeof(float)) return FieldKind.Float;
            if (type == typeof(double)) return FieldKind.Double;
            if (type == typeof(string)) return FieldKind.String;
            if (type == typeof(Vector2)) return FieldKind.Vector2;
            if (type == typeof(Vector3)) return FieldKind.Vector3;
            if (type == typeof(Vector4)) return FieldKind.Vector4;
            if (typeof(Entity).IsAssignableFrom(type)) return FieldKind.Entity;
            if (type.IsEnum) return FieldKind.Enum;
            if (type == typeof(Prefab)) return FieldKind.Prefab;
            return null;
        }

        // The public instance fields of a kind the engine knows, a base class's first, each class's in the order they are
        // declared (readonly ones are left out: they can't be set)
        internal static List<FieldInfo> GetFields(Type type)
        {
            var chain = new List<Type>();
            for (Type? t = type; t != null && t != typeof(Entity); t = t.BaseType) chain.Insert(0, t);
            var fields = new List<FieldInfo>();
            foreach (Type t in chain)
            {
                var declared = new List<FieldInfo>(t.GetFields(BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly));
                declared.Sort((a, b) => a.MetadataToken.CompareTo(b.MetadataToken));
                foreach (FieldInfo field in declared)
                    if (!field.IsInitOnly && KindOf(field.FieldType) != null) fields.Add(field);
            }
            return fields;
        }

        static FieldInfo? Find(Entity script, string name)
        {
            foreach (FieldInfo field in GetFields(script.GetType()))
                if (field.Name == name) return field;
            return null;
        }

        // Each script class of the game, then each of its fields with its value in a new instance (the constructor runs
        // once per class for that: outside the game, so the engine's API isn't there)
        internal static int Describe(Assembly game, IntPtr user, delegate* unmanaged<IntPtr, NativeFieldReport*, void> report)
        {
            Type?[] types;
            try { types = game.GetTypes(); }
            catch (ReflectionTypeLoadException e) { types = e.Types; }

            int count = 0;
            foreach (Type? type in types)
            {
                if (type == null || !IsScript(type) || type.FullName == null) continue;
                object? defaults = null;
                try { defaults = Activator.CreateInstance(type); }
                catch (Exception e)
                {
                    Log.Warn($"{type.FullName}: its constructor threw, its fields show the type's default values instead " +
                        $"({(e.InnerException ?? e).Message})");
                }
                Report(user, report, type.FullName, null, null);
                foreach (FieldInfo field in GetFields(type)) Report(user, report, type.FullName, field, defaults);
                count++;
            }
            return count;
        }

        static void Report(IntPtr user, delegate* unmanaged<IntPtr, NativeFieldReport*, void> report, string className, FieldInfo? field,
            object? defaults)
        {
            var info = new NativeFieldReport();
            string name = "", typeName = "", enumNames = "", text = "";
            long[] enumValues = Array.Empty<long>();
            if (field != null)
            {
                FieldKind kind = KindOf(field.FieldType)!.Value;
                object? value = defaults != null ? field.GetValue(defaults) :
                    field.FieldType.IsValueType ? Activator.CreateInstance(field.FieldType) : null;
                ToNative(kind, value, &info.Default);
                name = field.Name;
                typeName = kind switch
                {
                    FieldKind.Bool => "bool", FieldKind.Int => "int", FieldKind.Float => "float", FieldKind.Double => "double",
                    FieldKind.String => "string", _ => field.FieldType.Name,
                };
                if (kind == FieldKind.String) text = value as string ?? "";
                if (kind == FieldKind.Prefab) text = (value as Prefab)?.Path ?? "";
                if (kind == FieldKind.Enum)
                {
                    enumNames = string.Join('\n', Enum.GetNames(field.FieldType));
                    Array values = Enum.GetValuesAsUnderlyingType(field.FieldType);
                    enumValues = new long[values.Length];
                    for (int i = 0; i < values.Length; i++) enumValues[i] = Convert.ToInt64(values.GetValue(i));
                }
            }
            fixed (char* classChars = className, nameChars = name, typeChars = typeName, enumChars = enumNames, textChars = text)
            fixed (long* values = enumValues)
            {
                info.Class = classChars;
                info.ClassLength = className.Length;
                if (field != null)
                {
                    info.Name = nameChars;
                    info.NameLength = name.Length;
                    info.TypeName = typeChars;
                    info.TypeNameLength = typeName.Length;
                    info.EnumNames = enumChars;
                    info.EnumNamesLength = enumNames.Length;
                    info.EnumValues = values;
                    info.EnumCount = enumValues.Length;
                    info.Default.Text = textChars;
                    info.Default.TextLength = text.Length;
                }
                report(user, &info);
            }
        }

        // A value into the C++ struct (a String's text is the caller's: it needs a pinned buffer)
        static void ToNative(FieldKind kind, object? value, NativeValue* native)
        {
            native->Type = (int)kind;
            switch (kind)
            {
                case FieldKind.Bool: native->Integer = value is true ? 1 : 0; break;
                case FieldKind.Int: native->Integer = value is int i ? i : 0; break;
                case FieldKind.Float: native->Number = value is float f ? f : 0; break;
                case FieldKind.Double: native->Number = value is double d ? d : 0; break;
                case FieldKind.Vector2: native->Vector = value is Vector2 v2 ? new Vector4(v2, 0, 0) : default; break;
                case FieldKind.Vector3: native->Vector = value is Vector3 v3 ? new Vector4(v3, 0) : default; break;
                case FieldKind.Vector4: native->Vector = value is Vector4 v4 ? v4 : default; break;
                case FieldKind.Entity: native->Entity = (value as Entity)?.Id ?? 0; break;
                case FieldKind.Enum: native->Integer = value != null ? Convert.ToInt64(value) : 0; break;
            }
        }

        // The value C++ gives a field: false (logged) when the field doesn't exist or is of another kind
        internal static bool Set(Entity script, string name, NativeValue* native)
        {
            FieldInfo? field = Find(script, name);
            if (field == null)
            {
                Log.Warn($"{script.GetType().Name} has no public field {name} (any more): its value in the scene isn't used");
                return false;
            }
            FieldKind kind = KindOf(field.FieldType)!.Value;
            if ((int)kind != native->Type)
            {
                Log.Warn($"{script.GetType().Name}.{name} is of another type now ({field.FieldType.Name}): its value in the scene isn't used");
                return false;
            }
            object? value = kind switch
            {
                FieldKind.Bool => native->Integer != 0,
                FieldKind.Int => (int)native->Integer,
                FieldKind.Float => (float)native->Number,
                FieldKind.Double => native->Number,
                FieldKind.String => native->Text != null ? new string(native->Text, 0, native->TextLength) : "",
                FieldKind.Vector2 => new Vector2(native->Vector.X, native->Vector.Y),
                FieldKind.Vector3 => new Vector3(native->Vector.X, native->Vector.Y, native->Vector.Z),
                FieldKind.Vector4 => native->Vector,
                FieldKind.Entity => Interop.EntityFor(native->Entity),
                FieldKind.Enum => Enum.ToObject(field.FieldType, native->Integer),
                FieldKind.Prefab => native->Text != null && native->TextLength > 0 ? new Prefab(new string(native->Text, 0, native->TextLength)) : null,
                _ => null,
            };
            // A field of a script class (public PlayerController Player): the entity must have that script
            if (kind == FieldKind.Entity && value != null && !field.FieldType.IsInstanceOfType(value))
            {
                Log.Warn($"{script.GetType().Name}.{name}: {value} has no {field.FieldType.Name} script, the field stays null");
                value = null;
            }
            field.SetValue(script, value);
            return true;
        }

        // A field's value now (the Inspector while the game runs). A string longer than C++'s room: only its length
        internal static bool Get(Entity script, string name, NativeValue* native)
        {
            FieldInfo? field = Find(script, name);
            if (field == null) return false;
            FieldKind kind = KindOf(field.FieldType)!.Value;
            object? value = field.GetValue(script);
            ToNative(kind, value, native);
            if (kind == FieldKind.String || kind == FieldKind.Prefab)
            {
                string text = (kind == FieldKind.Prefab ? (value as Prefab)?.Path : value as string) ?? "";
                native->TextLength = text.Length;
                if (native->Text != null && text.Length <= native->TextCapacity)
                    text.AsSpan().CopyTo(new Span<char>(native->Text, native->TextCapacity));
            }
            return true;
        }
    }
}
