/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/UUID_H2M.h"
#include "H2M/Core/RefH2M.h"
#include "AssetTypesH2M.h"

#include <entt.hpp>


namespace H2M {

	using AssetHandleH2M = UUID_H2M;

	class AssetH2M : public RefCountedH2M
	{
	public:
		AssetHandleH2M Handle = AssetHandleH2M{};
		uint16_t Flags = (uint16_t)AssetFlagH2M::None;

		virtual ~AssetH2M() {}

		static AssetTypeH2M GetStaticType() { return AssetTypeH2M::None; }
		virtual AssetTypeH2M GetAssetType() const { return AssetTypeH2M::None; }

		bool IsValid() const { return ((Flags & (uint16_t)AssetFlagH2M::Missing) | (Flags & (uint16_t)AssetFlagH2M::Invalid)) == 0; }

		virtual bool operator==(const AssetH2M& other) const
		{
			return Handle == other.Handle;
		}

		virtual bool operator!=(const AssetH2M& other) const
		{
			return !(*this == other);
		}

		bool IsFlagSet(AssetFlagH2M flag) const { return (uint16_t)flag & Flags; }
		void SetFlag(AssetFlagH2M flag, bool value = true)
		{
			if (value)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}
	};


}
