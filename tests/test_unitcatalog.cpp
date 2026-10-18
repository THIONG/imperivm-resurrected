// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>
#include <vector>

#include "game/unitcatalog.h"

namespace {

const CatalogUnit *Find(const UnitCatalog &c, const std::string &id)
{
	for (const CatalogUnit &u : c.units) {
		if (u.id == id) return &u;
	}
	return nullptr;
}

void Paths()
{
	EditTreeSpot s;
	CHECK(EditTreeUnit("Units/Greece/Hoplite", s));
	CHECK_EQ(s.folder, std::string("Greece"));
	CHECK_EQ(s.leaf, std::string("Hoplite"));
	CHECK_EQ(s.leafContext, std::string("editortree/Units/Greece"));
	CHECK(EditTreeUnit("units/Roman/Heroes/Caesar", s));
	CHECK_EQ(s.folder, std::string("Roman"));
	CHECK_EQ(s.leaf, std::string("Caesar"));
	CHECK_EQ(s.leafContext, std::string("editortree/units/Roman/Heroes"));
	CHECK(!EditTreeUnit("Units/Greece", s));
	CHECK(!EditTreeUnit("Structures/Stronghold (Gaul)/Townhall", s));
	CHECK(!EditTreeUnit("Units//Hoplite", s));
	CHECK(!EditTreeUnit("", s));
}

void SameNameUsesTheEditorName()
{
	const UnitCatalog c = BuildUnitCatalog({
		{"RHastatus", "Legionario", "Legionario", "Roma", 1, false},
		{"RHastatusPunic", "Legionario", "Legionario (púnico)", "Roma", 1, false},
		{"GHastatusTwin", "Legionario", "Legionario", "Roma", 1, false},
	});
	CHECK_EQ(c.groups.size(), static_cast<size_t>(1));
	const CatalogUnit *punic = Find(c, "RHastatusPunic");
	CHECK(punic && punic->name == "Legionario (púnico)");
	const CatalogUnit *a = Find(c, "RHastatus"), *b = Find(c, "GHastatusTwin");
	CHECK(a && b && a->name != b->name);
	CHECK(a && a->name == "Legionario (RHastatus)");
}

void GroupsAndOrder()
{
	const UnitCatalog c = BuildUnitCatalog({
		{"Wolf", "Lobo", "Lobo", "Animales", 1000, false},
		{"HHoplite", "Hoplita", "Hoplita", "Grecia", 4, false},
		{"GreekHero1", "Aquiles", "Héroe 1", "Grecia", 4, true},
		{"HArcher", "Arquero", "Arquero", "Grecia", 4, false},
		{"GArcher", "Arquero", "Arquero", "Galia", 0, false},
		{"DArcher", "Arquero", "Arquero", "Dacia", 110, false},
		{"Bear", "Oso", "Oso", "animales", 1000, false},
		{"Zeta", "Z", "Z", "Otros", 1000, false},
		{"NoGroup", "X", "X", "", 0, false},
		{"", "Y", "Y", "Galia", 0, false},
	});
	const std::vector<std::string> groups = {"Galia", "Grecia", "Dacia", "Animales", "Otros"};
	CHECK(c.groups == groups);
	CHECK_EQ(c.units.size(), static_cast<size_t>(8));
	if (c.units.size() == 8) {
		CHECK_EQ(c.units[0].id, std::string("GArcher"));
		CHECK_EQ(c.units[1].id, std::string("HArcher"));
		CHECK_EQ(c.units[2].id, std::string("HHoplite"));
		CHECK_EQ(c.units[3].id, std::string("GreekHero1"));
		CHECK(c.units[3].hero);
		CHECK_EQ(c.units[4].id, std::string("DArcher"));
		CHECK_EQ(c.units[5].id, std::string("Wolf"));
		CHECK_EQ(c.units[6].id, std::string("Bear"));
		CHECK_EQ(c.units[5].group, 3);
		CHECK_EQ(c.units[6].group, 3);
	}
}

void MissingNames()
{
	const UnitCatalog c = BuildUnitCatalog({
		{"A", "**Invalid**", "Leaf A", "G", 0, false},
		{"B", "", "", "G", 0, false},
	});
	const CatalogUnit *a = Find(c, "A"), *b = Find(c, "B");
	CHECK(a && a->name == "Leaf A");
	CHECK(b && b->name == "B");
}

}

int main()
{
	Paths();
	SameNameUsesTheEditorName();
	GroupsAndOrder();
	MissingNames();
	return test::Summary("unitcatalog");
}
