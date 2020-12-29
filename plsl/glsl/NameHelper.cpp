/*
 * Microsoft Public License (Ms-PL) - Copyright (c) 2020 Sean Moss
 * This file is subject to the terms and conditions of the Microsoft Public License, the text of which can be found in
 * the 'LICENSE' file at the root of this repository, or online at <https://opensource.org/licenses/MS-PL>.
 */

#include "./NameHelper.hpp"


namespace plsl
{

// ====================================================================================================================
string NameHelper::GetNumericTypeName(ShaderBaseType type, uint8 size, uint8 dim0, uint8 dim1)
{
	// TODO: Add more once non-4 sizes are supported
	switch (type)
	{
	case ShaderBaseType::Boolean:
		return (dim0 == 1) ? "bool" : (dim0 == 2) ? "bvec2" : (dim0 == 3) ? "bvec3" : (dim0 == 4) ? "bvec4" : "";
	case ShaderBaseType::Unsigned:
		return (dim0 == 1) ? "uint" : (dim0 == 2) ? "uvec2" : (dim0 == 3) ? "uvec3" : (dim0 == 4) ? "uvec4" : "";
	case ShaderBaseType::Signed:
		return (dim0 == 1) ? "int" : (dim0 == 2) ? "ivec2" : (dim0 == 3) ? "ivec3" : (dim0 == 4) ? "ivec4" : "";
	case ShaderBaseType::Float: {
		if (dim1 == 1) {
			return (dim0 == 1) ? "float" : (dim0 == 2) ? "vec2" : (dim0 == 3) ? "vec3" : (dim0 == 4) ? "vec4" : "";
		}
		if (dim1 == 2) {
			return (dim0 == 2) ? "mat2x2" : (dim0 == 3) ? "mat2x3" : (dim0 == 4) ? "mat2x4" : "";
		}
		if (dim1 == 3) {
			return (dim0 == 2) ? "mat3x2" : (dim0 == 3) ? "mat3x3" : (dim0 == 4) ? "mat3x4" : "";
		}
		if (dim1 == 4) {
			return (dim0 == 2) ? "mat4x2" : (dim0 == 3) ? "mat4x3" : (dim0 == 4) ? "mat4x4" : "";
		}
	} break;
	}

	return "";
}

// ====================================================================================================================
string NameHelper::GetBindingTypeName(const ShaderType* type, string* extra)
{
	switch (type->baseType)
	{
	case ShaderBaseType::Sampler: {
		const auto dimText = GetImageDimsPostfix(type->image.dims);
		if (dimText.empty()) {
			return "";
		}
		const string prefix = GetImageTexelPrefix(type->image.texel.type);
		*extra = "";
		return prefix + "sampler" + dimText;
	} break;
	case ShaderBaseType::Image: {
		const auto dimText = GetImageDimsPostfix(type->image.dims);
		if (dimText.empty()) {
			return "";
		}
		const string prefix = GetImageTexelPrefix(type->image.texel.type);
		*extra = GetImageTexelFormat(type->image.texel.type, type->image.texel.size, type->image.texel.components);
		if (extra->empty()) {
			*extra = "!";
		}
		return prefix + "image" + dimText;
	} break;
	case ShaderBaseType::Uniform: {
		*extra = "std140";
		return " ";
	} break;
	case ShaderBaseType::ROBuffer: {
		*extra = "std430";
		return " ";
	} break;
	case ShaderBaseType::RWBuffer: {
		*extra = "std430";
		return " ";
	} break;
	case ShaderBaseType::ROTexels: {
		*extra = "";
		return "textureBuffer";
	} break;
	case ShaderBaseType::RWTexels: {
		*extra = GetImageTexelFormat(type->image.texel.type, type->image.texel.size, type->image.texel.components);
		return "imageBuffer";
	} break;
	}
	
	*extra = "";
	return "";
}

// ====================================================================================================================
string NameHelper::GetImageDimsPostfix(ImageDims dims)
{
	switch (dims)
	{
	case ImageDims::E1D: return "1D";
	case ImageDims::E2D: return "2D";
	case ImageDims::E3D: return "3D";
	case ImageDims::E1DArray: return "1DArray";
	case ImageDims::E2DArray: return "2DArray";
	case ImageDims::Cube: return "Cube";
	default: return "";
	}
}

// ====================================================================================================================
string NameHelper::GetImageTexelPrefix(ShaderBaseType type)
{
	switch (type)
	{
	case ShaderBaseType::Unsigned: return "u";
	case ShaderBaseType::Signed: return "i";
	default: return "";
	}
}

// ====================================================================================================================
string NameHelper::GetImageTexelFormat(ShaderBaseType type, uint8 size, uint8 dim0)
{
	switch (type)
	{
	case ShaderBaseType::Unsigned:
		return (dim0 == 1) ? "r32ui" : (dim0 == 2) ? "rg32ui" : (dim0 == 4) ? "rgba32ui" : "";
	case ShaderBaseType::Signed:
		return (dim0 == 1) ? "r32i" : (dim0 == 2) ? "rg32i" : (dim0 == 4) ? "rgba32i" : "";
	case ShaderBaseType::Float: {
		if (size == 4) {
			return (dim0 == 1) ? "r32f" : (dim0 == 2) ? "rg32f" : (dim0 == 4) ? "rgba32f" : "";
		}
		else if (size == 1) {
			return (dim0 == 1) ? "r8" : (dim0 == 2) ? "rg8" : (dim0 == 4) ? "rgba8" : "";
		}
		return "";
	}
	default: return "";
	}
}

} // namespace plsl
