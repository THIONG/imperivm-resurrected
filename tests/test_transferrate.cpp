// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include "core/transferrate.h"

namespace {

uint64_t Rate(const TransferRate &r) { return static_cast<uint64_t>(r.perSecond + 0.5); }

void MeasuresBytesPerSecond()
{
	TransferRate r;
	r.Sample(0, 1000);
	CHECK_EQ(Rate(r), 0ull);
	r.Sample(500000, 1500);
	CHECK_EQ(Rate(r), 0ull);
	r.Sample(1000000, 2000);
	CHECK_EQ(Rate(r), 1000000ull);
	r.Sample(2000000, 3000);
	CHECK_EQ(Rate(r), 1000000ull);
}

void SlowsDownWhenTheDataStops()
{
	TransferRate r;
	r.Sample(0, 0);
	r.Sample(1000000, 1000);
	r.Sample(1000000, 2000);
	CHECK_EQ(Rate(r), 600000ull);
	r.Sample(1000000, 3000);
	CHECK_EQ(Rate(r), 360000ull);
}

void StartsAgainWithTheNextFile()
{
	TransferRate r;
	r.Sample(0, 0);
	r.Sample(4000000, 1000);
	r.Sample(100, 1200);
	CHECK_EQ(Rate(r), 0ull);
	r.Sample(200100, 2200);
	CHECK_EQ(Rate(r), 200000ull);
}

void SurvivesTheTickWrapping()
{
	TransferRate r;
	r.Sample(0, 0xFFFFFF00u);
	r.Sample(1000000, 0xFFFFFF00u + 1000u);
	CHECK_EQ(Rate(r), 1000000ull);
}

}

int main()
{
	MeasuresBytesPerSecond();
	SlowsDownWhenTheDataStops();
	StartsAgainWithTheNextFile();
	SurvivesTheTickWrapping();
	return test::Summary("transferrate");
}
