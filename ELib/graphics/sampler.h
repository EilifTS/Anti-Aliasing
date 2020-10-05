#pragma once
#include "internal/egx_common.h"

namespace egx
{
	class Sampler
	{
	public:
		enum class Filter
		{
			Point, Linear
		};
		enum class AddressMode
		{
			Border, Clamp, Mirror, Wrap
		};

	public:
		inline Sampler() 
			: desc() 
		{
			desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			desc.MipLODBias = 0.0;
			desc.MaxAnisotropy = 0;
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			desc.MinLOD = 0.0f;
			desc.MaxLOD = D3D12_FLOAT32_MAX;
			desc.ShaderRegister = 0;
			desc.RegisterSpace = 0;
			desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		};

		inline void SetFilter(Filter filter)
		{
			auto filter_x = convertFilter(filter);
			desc.Filter = filter_x;
		};
		inline void SetAddressMode(AddressMode address_mode)
		{
			auto address_mode_x = convertAddressMode(address_mode);
			desc.AddressU = address_mode_x;
			desc.AddressV = address_mode_x;
			desc.AddressW = address_mode_x;
		};
		inline void SetMipMapBias(float bias)
		{
			desc.MipLODBias = bias;
		};
		void SetVisibility(ShaderVisibility visibility);

		inline static Sampler LinearClamp()
		{
			Sampler s;
			s.SetAddressMode(AddressMode::Clamp);
			s.SetFilter(Filter::Linear);
			return s;
		}
		inline static Sampler LinearWrap()
		{
			Sampler s;
			s.SetAddressMode(AddressMode::Wrap);
			s.SetFilter(Filter::Linear);
			return s;
		}
		inline static Sampler PointWrap()
		{
			Sampler s;
			s.SetAddressMode(AddressMode::Wrap);
			s.SetFilter(Filter::Point);
			return s;
		}
		inline static Sampler PointClamp()
		{
			Sampler s;
			s.SetAddressMode(AddressMode::Clamp);
			s.SetFilter(Filter::Point);
			return s;
		}
		inline static Sampler ShadowSampler()
		{
			Sampler s;
			s.desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
			s.desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			s.SetAddressMode(AddressMode::Clamp);
			return s;
		}

	private:
		D3D12_STATIC_SAMPLER_DESC desc;

	private:
		inline D3D12_FILTER convertFilter(Filter filter)
		{
			switch (filter)
			{
			case Filter::Linear: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			case Filter::Point: return D3D12_FILTER_MIN_MAG_MIP_POINT;
			}
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
		};
		inline D3D12_TEXTURE_ADDRESS_MODE convertAddressMode(AddressMode address_mode)
		{
			switch (address_mode)
			{
			case AddressMode::Border: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			case AddressMode::Clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			case AddressMode::Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			case AddressMode::Wrap: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			}
			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		};
		friend RootSignature;
	};
}