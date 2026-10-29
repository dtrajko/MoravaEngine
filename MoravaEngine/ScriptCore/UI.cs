using System;
using System.Collections.Generic;
using System.Numerics;

namespace Morava
{
    /// <summary>How a text's lines sit in its rectangle, left to right.</summary>
    public enum TextAlign
    {
        /// <summary>Each line starts at the rectangle's left edge.</summary>
        Left = 0,
        /// <summary>Each line centered.</summary>
        Center = 1,
        /// <summary>Each line ends at the rectangle's right edge.</summary>
        Right = 2
    }

    /// <summary>How a text's lines sit in its rectangle, top to bottom.</summary>
    public enum TextVerticalAlign
    {
        /// <summary>The first line at the rectangle's top.</summary>
        Top = 0,
        /// <summary>The lines centered.</summary>
        Middle = 1,
        /// <summary>The last line at the rectangle's bottom.</summary>
        Bottom = 2
    }

    /// <summary>
    /// Where a UI element is in its parent's rectangle (Inspector: Rectangle), in the canvas's reference pixels with x to
    /// the right and y down. The anchors are points of the parent, (0, 0) top-left to (1, 1) bottom-right: at one point the
    /// element keeps its Size at Position from it; apart, it stretches with the parent and Size is added to the span.
    /// </summary>
    public sealed unsafe class UIRect : Component
    {
        NativeUIRect Rect
        {
            get
            {
                NativeUIRect rect;
                if (Interop.Game->UIRect_Get(Entity.Id, &rect) == 0) throw Missing("UI rectangle");
                return rect;
            }
            set
            {
                if (Interop.Game->UIRect_Set(Entity.Id, &value) == 0) throw Missing("UI rectangle");
            }
        }

        /// <summary>False hides the element and everything under it (a closed menu).</summary>
        public bool Visible { get => Rect.Visible != 0; set { var r = Rect; r.Visible = value ? 1 : 0; Rect = r; } }

        /// <summary>From the anchors to the pivot, in reference pixels (x right, y down).</summary>
        public Vector2 Position { get => Rect.Position; set { var r = Rect; r.Position = value; Rect = r; } }

        /// <summary>Reference pixels; along a stretched side, added to the span between the anchors.</summary>
        public Vector2 Size { get => Rect.Size; set { var r = Rect; r.Size = value; Rect = r; } }

        /// <summary>The anchors' corner nearer the parent's top-left, 0 to 1.</summary>
        public Vector2 AnchorMin { get => Rect.AnchorMin; set { var r = Rect; r.AnchorMin = value; Rect = r; } }

        /// <summary>The anchors' corner nearer the parent's bottom-right, 0 to 1.</summary>
        public Vector2 AnchorMax { get => Rect.AnchorMax; set { var r = Rect; r.AnchorMax = value; Rect = r; } }

        /// <summary>The element's own point that Position places, and that it grows from: (0.5, 0.5) its center.</summary>
        public Vector2 Pivot { get => Rect.Pivot; set { var r = Rect; r.Pivot = value; Rect = r; } }
    }

    /// <summary>
    /// Text in the element's rectangle (Inspector: Text): scoreText.Text = $"Score {score}". Sharp at any size. Lines
    /// break at '\n', and with Wrap also between words.
    /// </summary>
    public sealed unsafe class UIText : Component
    {
        NativeUITextStyle Style
        {
            get
            {
                NativeUITextStyle style;
                if (Interop.Game->UIText_GetStyle(Entity.Id, &style) == 0) throw Missing("UI text");
                return style;
            }
            set
            {
                if (Interop.Game->UIText_SetStyle(Entity.Id, &value) == 0) throw Missing("UI text");
            }
        }

        /// <summary>The text shown.</summary>
        public string Text
        {
            get => UIStrings.Get(Entity, "UI text", (char* buffer, int capacity) => Interop.Game->UIText_GetText(Entity.Id, buffer, capacity));
            set
            {
                string text = value ?? "";
                fixed (char* chars = text)
                    if (Interop.Game->UIText_SetText(Entity.Id, chars, text.Length) == 0) throw Missing("UI text");
            }
        }

        /// <summary>The font file (.ttf or .otf, relative to the project); "" for the engine's font.</summary>
        public string Font
        {
            get => UIStrings.Get(Entity, "UI text", (char* buffer, int capacity) => Interop.Game->UIText_GetFont(Entity.Id, buffer, capacity));
            set
            {
                string font = value ?? "";
                fixed (char* chars = font)
                    if (Interop.Game->UIText_SetFont(Entity.Id, chars, font.Length) == 0) throw Missing("UI text");
            }
        }

        /// <summary>The font's em size in reference pixels.</summary>
        public float Size { get => Style.Size; set { var s = Style; s.Size = value; Style = s; } }

        /// <summary>Red, green, blue and alpha, 0 to 1, as they appear on the screen.</summary>
        public Vector4 Color { get => Style.Color; set { var s = Style; s.Color = value; Style = s; } }

        /// <summary>Left, centered or right in the rectangle.</summary>
        public TextAlign Align { get => (TextAlign)Style.Align; set { var s = Style; s.Align = (int)value; Style = s; } }

        /// <summary>At the rectangle's top, in its middle or at its bottom.</summary>
        public TextVerticalAlign VerticalAlign { get => (TextVerticalAlign)Style.VerticalAlign; set { var s = Style; s.VerticalAlign = (int)value; Style = s; } }

        /// <summary>Lines also break between words where they would leave the rectangle.</summary>
        public bool Wrap { get => Style.Wrap != 0; set { var s = Style; s.Wrap = value ? 1 : 0; Style = s; } }

        /// <summary>Times the font's own line height.</summary>
        public float LineSpacing { get => Style.LineSpacing; set { var s = Style; s.LineSpacing = value; Style = s; } }
    }

    /// <summary>
    /// A picture filling the element's rectangle, tinted by Color (Inspector: Image); without a texture a plain
    /// rectangle of that color: a panel, a health bar.
    /// </summary>
    public sealed unsafe class UIImage : Component
    {
        /// <summary>Red, green, blue and alpha, 0 to 1: multiplies the texture.</summary>
        public Vector4 Color
        {
            get { Vector4 color; if (Interop.Game->UIImage_GetColor(Entity.Id, &color) == 0) throw Missing("UI image"); return color; }
            set { if (Interop.Game->UIImage_SetColor(Entity.Id, &value) == 0) throw Missing("UI image"); }
        }

        /// <summary>The image file (relative to the project); "" for none.</summary>
        public string Texture
        {
            get => UIStrings.Get(Entity, "UI image", (char* buffer, int capacity) => Interop.Game->UIImage_GetTexture(Entity.Id, buffer, capacity));
            set
            {
                string texture = value ?? "";
                fixed (char* chars = texture)
                    if (Interop.Game->UIImage_SetTexture(Entity.Id, chars, texture.Length) == 0) throw Missing("UI image");
            }
        }
    }

    /// <summary>
    /// A button (Inspector: Button): clicked with the mouse, or selected with UI_Navigate (the arrow keys, the D-pad) and
    /// pressed with UI_Submit (Enter, the gamepad's A). Its image shows its state's color. A click calls the method the
    /// Inspector names (On Click), then everything listening: button.Clicked += OnPlay.
    /// </summary>
    public sealed unsafe class UIButton : Component
    {
        // The listeners of each button, by entity: a button is asked for anew each time (GetComponent), its listeners stay
        static readonly Dictionary<ulong, Action?> s_Clicked = new();

        /// <summary>False: the button shows its Disabled color, can't be clicked or selected.</summary>
        public bool Interactable
        {
            get { int value; if (Interop.Game->UIButton_GetInteractable(Entity.Id, &value) == 0) throw Missing("UI button"); return value != 0; }
            set { if (Interop.Game->UIButton_SetInteractable(Entity.Id, value ? 1 : 0) == 0) throw Missing("UI button"); }
        }

        /// <summary>The button is the one UI_Submit presses (selected by the keyboard, the gamepad, the mouse or Select).</summary>
        public bool IsSelected => Interop.Game->UI_GetSelected() == Entity.Id;

        /// <summary>
        /// Makes it the selected button (a menu that opens: its first button, so the gamepad can press it). False when it
        /// can't be selected now: hidden, or not Interactable.
        /// </summary>
        public bool Select() => Interop.Game->UI_Select(Entity.Id) != 0;

        /// <summary>After the button's On Click method, in the frame's update (before OnUpdate), also while TimeScale is 0.</summary>
        public event Action Clicked
        {
            add { s_Clicked.TryGetValue(Entity.Id, out Action? listeners); s_Clicked[Entity.Id] = listeners + value; }
            remove
            {
                if (s_Clicked.TryGetValue(Entity.Id, out Action? listeners))
                {
                    listeners -= value;
                    if (listeners == null) s_Clicked.Remove(Entity.Id); else s_Clicked[Entity.Id] = listeners;
                }
            }
        }

        // Each listener, also when one before it threw (logged): true when none threw
        internal static bool RaiseClicked(ulong button)
        {
            if (!s_Clicked.TryGetValue(button, out Action? listeners) || listeners == null) return true;
            bool ok = true;
            foreach (Action listener in listeners.GetInvocationList())
            {
                try { listener(); }
                catch (Exception e)
                {
                    Log.Error($"{listener.Method.DeclaringType?.Name}.{listener.Method.Name} (UIButton.Clicked): {e}");
                    ok = false;
                }
            }
            return ok;
        }

        internal static void ClearListeners() => s_Clicked.Clear();
    }

    /// <summary>The game UI as a whole.</summary>
    public static unsafe class UI
    {
        /// <summary>The selected button (UI_Submit presses it), null for none; set null to drop the selection.</summary>
        public static UIButton? SelectedButton
        {
            get
            {
                ulong id = Interop.Game->UI_GetSelected();
                return id == 0 ? null : new UIButton { Entity = Interop.EntityFor(id)! };
            }
            set
            {
                if (value == null) Interop.Game->UI_Select(0);
                else value.Select();
            }
        }

        /// <summary>
        /// The mouse is over an element that catches it (an image or a button): a game reading the mouse itself can leave
        /// it to the UI. A press that starts there doesn't reach the game's mouse actions anyway.
        /// </summary>
        public static bool IsPointerOverUI => Interop.Game->UI_IsPointerOver() != 0;
    }

    // A string the engine copies into a buffer: the length, -1 when it doesn't fit (a larger buffer then), -2 when the
    // entity has no such component
    static unsafe class UIStrings
    {
        internal delegate int Copy(char* buffer, int capacity);

        internal static string Get(Entity entity, string what, Copy copy)
        {
            char* small = stackalloc char[512];
            int length = copy(small, 512);
            for (int capacity = 8192; length == -1 && capacity <= 1 << 24; capacity *= 8)
            {
                char[] large = new char[capacity];
                fixed (char* chars = large)
                    length = copy(chars, capacity);
                if (length >= 0) return new string(large, 0, length);
            }
            if (length == -2) throw new InvalidOperationException($"{entity} has no {what} in the running game");
            return length >= 0 ? new string(small, 0, length) : "";
        }
    }
}
