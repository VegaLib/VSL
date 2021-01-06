/*
 * Microsoft Public License (Ms-PL) - Copyright (c) 2020-2021 Sean Moss
 * This file is subject to the terms and conditions of the Microsoft Public License, the text of which can be found in
 * the 'LICENSE' file at the root of this repository, or online at <https://opensource.org/licenses/MS-PL>.
 */

#include "./Generator.hpp"
#include "./NameHelper.hpp"

#include <fstream>
#include <iostream>

#define MAKE_ERROR(str) CompilerError(CompilerStage::Generate, str, 0, 0)


namespace vsl
{

// ====================================================================================================================
Generator::Generator(const BindingTableSizes& tableSizes)
	: tableSizes_{ tableSizes }
	, globals_{ }
	, stageHeaders_{ }
	, stageFunctions_{ }
	, currentFunc_{ nullptr }
	, uniqueId_{ 0 }
	, localId_{ 0 }
	, indentString_{ "" }
	, bindingEmitMask_{ 0 }
{
	// Setup initial globals content
	globals_
		<< "/// This file generated by vslc\n"
		<< "#version 450 core\n"
		<< "#extension GL_EXT_scalar_block_layout : require\n"
		<< "\n";
}

// ====================================================================================================================
Generator::~Generator()
{

}

// ====================================================================================================================
bool Generator::getStageString(ShaderStages stage, string* outstring) const
{
	// Get the stage-specific header and body
	const auto stageName = ShaderStageToStr(stage);
	const auto hit = stageHeaders_.find(stageName);
	const auto fit = stageFunctions_.find(stageName);
	if ((hit == stageHeaders_.end()) || (fit == stageFunctions_.end())) {
		*outstring = "";
		return false;
	}

	// Build the string
	const auto gstr = globals_.str();
	const auto hstr = hit->second.str();
	const auto fstr = fit->second.str();
	outstring->resize(gstr.length() + hstr.length() + fstr.length(), '\0');
	std::memcpy(outstring->data(), gstr.data(), gstr.length());
	std::memcpy(outstring->data() + gstr.length(), hstr.data(), hstr.length());
	std::memcpy(outstring->data() + gstr.length() + hstr.length(), fstr.data(), fstr.length());
	return true;
}

// ====================================================================================================================
void Generator::setCurrentStage(ShaderStages stage)
{
	if (stage == ShaderStages::None) {
		*currentFunc_ << "}\n" << std::endl;
		currentFunc_ = nullptr;
		indentString_ = "";
	}
	else {
		const auto name = ShaderStageToStr(stage);
		if (stageHeaders_.find(name) == stageHeaders_.end()) {
			stageHeaders_[name] = std::stringstream();
		}
		currentFunc_ = &(stageFunctions_[name] = {});
		*currentFunc_ << "void main()\n{\n";
		indentString_ = "\t";
		bindingEmitMask_ = 0;
	}
}

// ====================================================================================================================
void Generator::emitStruct(const string& name, const std::vector<StructMember>& members)
{
	globals_ << "struct " << name << "_t {" << std::endl;
	for (const auto& mem : members) {
		const auto mtype = NameHelper::GetNumericTypeName(mem.baseType, mem.size, mem.dims[0], mem.dims[1]);
		if (mtype.empty()) {
			ERROR(mkstr("Unmappable numeric type [%u:%u:%ux%u]", 
				uint32(mem.baseType), uint32(mem.size), uint32(mem.dims[0]), uint32(mem.dims[1])));
		}
		const auto arr = (mem.arraySize != 1) ? mkstr("[%u]", uint32(mem.arraySize)) : "";
		globals_ << '\t' << mtype << ' ' << mem.name << arr << ";\n";
	}
	globals_ << "};\n" << std::endl;
}

// ====================================================================================================================
void Generator::emitVertexInput(const InterfaceVariable& var)
{
	// Check vertex header
	if (stageHeaders_.find("vert") == stageHeaders_.end()) {
		stageHeaders_["vert"] = std::stringstream();
	}
	auto& header = stageHeaders_["vert"];

	// Emit
	const auto vtype = NameHelper::GetNumericTypeName(var.type->baseType, var.type->numeric.size, 
		var.type->numeric.dims[0], var.type->numeric.dims[1]);
	const auto arr = (var.arraySize != 1) ? mkstr("[%u]", uint32(var.arraySize)) : "";
	header << "layout(location = " << var.location << ") in " << vtype << ' ' << var.name << arr << ";\n" << std::endl;
}

// ====================================================================================================================
void Generator::emitFragmentOutput(const InterfaceVariable& var)
{
	// Check fragment header
	if (stageHeaders_.find("frag") == stageHeaders_.end()) {
		stageHeaders_["frag"] = std::stringstream();
	}
	auto& header = stageHeaders_["frag"];

	// Emit
	const auto vtype = NameHelper::GetNumericTypeName(var.type->baseType, var.type->numeric.size,
		var.type->numeric.dims[0], var.type->numeric.dims[1]);
	header << "layout(location = " << var.location << ") out " << vtype << ' ' << var.name << ";\n" << std::endl;
}

// ====================================================================================================================
void Generator::emitUniform(const UniformVariable& unif)
{
	globals_ << "layout(set = 1, binding = 0, scalar) uniform _UBUFFER0_ {\n";
	globals_ << "\t" << unif.type->buffer.structType->userStruct.structName << "_t " << unif.name << ";\n";
	globals_ << "};\n" << std::endl;
}

// ====================================================================================================================
void Generator::emitBinding(const BindingVariable& bind)
{
	// Type info
	string extra{};
	const auto btype = NameHelper::GetBindingTypeName(bind.type, &extra);
	if (btype.empty()) {
		ERROR(mkstr("Unmappable binding type for '%s'", bind.name.c_str()));
	}
	if (extra == "!") {
		ERROR(mkstr("Invalid binding extra type for '%s'", bind.name.c_str()));
	}
	const auto tableName = NameHelper::GetBindingTableName(bind.type);

	// Get the set and binding
	uint32 set, binding;
	uint16 tableSize;
	getSetAndBinding(bind, &set, &binding, &tableSize);

	// Emit
	globals_ << "layout(set = " << set << ", binding = " << binding << (!extra.empty() ? (", " + extra) : "") << ") ";
	if (bind.type->isBuffer()) {
		globals_ << ((bind.type->baseType == ShaderBaseType::ROBuffer) ? "readonly " : "") << "buffer ";
		globals_ << "_BUFFER" << (uniqueId_++) << "_ {\n";
		globals_ << '\t' << bind.type->buffer.structType->userStruct.structName << "_t _data_[];\n";
		globals_ << "} " << bind.name << '[' << tableSize << ']';
	}
	else {
		globals_ << "uniform " << btype << ' ' << tableName << '[' << tableSize << ']';
	}
	globals_ << ";\n" << std::endl;
}

// ====================================================================================================================
void Generator::emitSubpassInput(const SubpassInput& input)
{
	// Check fragment header
	if (stageHeaders_.find("frag") == stageHeaders_.end()) {
		stageHeaders_["frag"] = std::stringstream();
	}
	auto& header = stageHeaders_["frag"];

	const string prefix =
		(input.type == ShaderBaseType::Unsigned) ? "u" :
		(input.type == ShaderBaseType::Signed) ? "i" : "";
	const auto index = uint32(input.index);
	header
		<< "layout(set = 2, binding = " << index << ", input_attachment_index = " << index << ") "
		<< "uniform " << prefix << "subpassInput " << input.name << ";\n" << std::endl;
}

// ====================================================================================================================
void Generator::emitLocal(const Variable& var)
{
	// Check headers
	if (stageHeaders_.find("vert") == stageHeaders_.end()) {
		stageHeaders_["vert"] = std::stringstream();
	}
	auto& vert = stageHeaders_["vert"];
	if (stageHeaders_.find("frag") == stageHeaders_.end()) {
		stageHeaders_["frag"] = std::stringstream();
	}
	auto& frag = stageHeaders_["frag"];

	// Get the type
	const auto type = NameHelper::GetNumericTypeName(var.dataType->baseType, var.dataType->numeric.size, 
		var.dataType->numeric.dims[0], 1);
	const string flat = var.extra.local.flat ? "flat " : "";
	
	// Emit
	vert
		<< "layout(location = " << localId_ << ") " << flat << "out " << type << " _vert_" << var.name << ";\n" 
		<< std::endl;
	frag
		<< "layout(location = " << localId_ << ") " << flat << "in " << type << " _frag_" << var.name << ";\n"
		<< std::endl;
	localId_ += 1;
}

// ====================================================================================================================
void Generator::emitBindingIndices(uint32 maxIndex)
{
	const auto icount = (maxIndex + 2) / 2; // +1 to shift index->count, +1 to perform correct floor operation

	// Emit
	globals_ << "layout (push_constant) uniform _BINDING_INDICES_ {\n";
	for (uint32 i = 0; i < icount; ++i) {
		globals_ << "\tuint index" << i << ";\n";
	}
	globals_ << "} _bidx_;\n\n";
}

// ====================================================================================================================
void Generator::emitDeclaration(const Variable& var)
{
	*currentFunc_ << indentString_ << NameHelper::GetGeneralTypeName(var.dataType) << " " << var.name << ";\n";
}

// ====================================================================================================================
void Generator::emitAssignment(const string& left, const string& op, const string& right)
{
	*currentFunc_ << indentString_ << left << ' ' << op << ' ' << right << ";\n";
}

// ====================================================================================================================
void Generator::emitImageStore(const string& imStore, const string& value)
{
	auto repl = imStore;
	const auto repidx = repl.find("{}");
	repl.replace(repidx, 2, value);
	*currentFunc_ << indentString_ << repl << ";\n";
}

// ====================================================================================================================
void Generator::emitBindingIndex(uint32 index)
{
	// Check if already emitted
	if (bindingEmitMask_ & (1u << index)) {
		return;
	}

	// Emit new binding index
	const auto bindstr = NameHelper::GetBindingIndexText(index);
	const auto bindname = mkstr("_bidx%u_", index);
	*currentFunc_ << indentString_ << "uint " << bindname << " = " << bindstr << ";\n";
	bindingEmitMask_ = (bindingEmitMask_ | (1u << index)); // Set flag
}

// ====================================================================================================================
void Generator::emitIf(const string& condition)
{
	*currentFunc_ << indentString_ << "if (" << condition << ") {\n";
	indentString_ += '\t';
}

// ====================================================================================================================
void Generator::emitElif(const string& condition)
{
	*currentFunc_ << indentString_ <<  "else if (" << condition << ") {\n";
	indentString_ += '\t';
}

// ====================================================================================================================
void Generator::emitElse()
{
	*currentFunc_ << indentString_ << "else {\n";
	indentString_ += '\t';
}

// ====================================================================================================================
void Generator::emitBlockClose()
{
	indentString_ = indentString_.substr(0, indentString_.length() - 1);
	*currentFunc_ << indentString_ << "}\n";
}

// ====================================================================================================================
void Generator::getSetAndBinding(const BindingVariable& bind, uint32* set, uint32* binding, uint16* tableSize)
{
	// Easy
	*set = 0;

	// Less easy
	switch (bind.type->baseType)
	{
	case ShaderBaseType::Sampler: *binding = 0; *tableSize = tableSizes_.samplers; break;
	case ShaderBaseType::Image: *binding = 1; *tableSize = tableSizes_.images; break;
	case ShaderBaseType::RWBuffer:
	case ShaderBaseType::ROBuffer: *binding = 2; *tableSize = tableSizes_.buffers; break;
	case ShaderBaseType::ROTexels: *binding = 3; *tableSize = tableSizes_.roTexels; break;
	case ShaderBaseType::RWTexels: *binding = 4; *tableSize = tableSizes_.rwTexels; break;

	default: ERROR("Invalid type for set and binding indices");
	}
}

} // namespace vsl
