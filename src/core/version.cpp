// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/version.h"
#include "core/version_info.h"

namespace {

const char kVersion[] = IMPERIVM_VERSION_TEXT;

}

const char *ModVersionText()
{
	return kVersion;
}
