/*
 * Microsoft Public License (Ms-PL) - Copyright (c) 2020 Sean Moss
 * This file is subject to the terms and conditions of the Microsoft Public License, the text of which can be found in
 * the 'LICENSE' file at the root of this repository, or online at <https://opensource.org/licenses/MS-PL>.
 */

#include "./ScopeManager.hpp"

#include <algorithm>


namespace plsl
{

// ====================================================================================================================
ScopeManager::ScopeManager()
	: allGlobals_{ }
{

}

// ====================================================================================================================
ScopeManager::~ScopeManager()
{

}

// ====================================================================================================================
bool ScopeManager::addGlobal(const Variable& var)
{
	if (hasGlobal(var.name())) {
		return false;
	}
	allGlobals_.push_back(var);
	return true;
}

// ====================================================================================================================
bool ScopeManager::hasGlobal(const string& name) const
{
	const auto it = std::find_if(allGlobals_.begin(), allGlobals_.end(), [&name](const Variable& var) {
		return var.name() == name;
	});
	return it != allGlobals_.end();
}

} // namespace plsl
