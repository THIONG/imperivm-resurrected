// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <deque>
#include <set>

#include "MinHook.h"
#include "hooks/hookcount.h"
#include <atomic>
#include <string>
#include <vector>
#include <cstdint>

#include "game/controls.h"
#include "game/gameroom.h"
#include "mods/modoptions.h"
#include "core/legacy.h"
#include "core/log.h"
#include "game/cheats.h"
#include "addr/engine.h"
#include "addr/state.h"
#include "addr/net.h"
#include "addr/script.h"
#include "addr/ui.h"
#include "addr/world.h"
#include "core/uiport.h"

namespace {
const char kPrefix[] = "#CHEAT:";
const size_t kPrefixLen = sizeof(kPrefix) - 1;

typedef void(__fastcall *ChatSend_t)(void *dlg);
typedef void *(__fastcall *GetChild_t)(void *dlg, void *edx, int id, const char *cls);
typedef int(__fastcall *GetText_t)(void *ctrl, void *edx, char *buf, int size);
typedef bool(__cdecl *Bool_t)();
typedef int(__cdecl *IsDemo_t)();
typedef void *(__cdecl *Create_t)();
typedef void(__fastcall *Assign_t)(void *self, void *edx, const char *text, unsigned len);
typedef void(__fastcall *ThisPtr_t)(void *self, void *edx, void *arg);
typedef void(__fastcall *RunScript_t)(void *game, void *edx, const char *text);

ChatSend_t g_origChatSend = nullptr;

void **Vt(void *obj) { return *reinterpret_cast<void ***>(obj); }
void *Game() { return *reinterpret_cast<void **>(addr::kGame); }

bool Allowed()
{
	if (reinterpret_cast<IsDemo_t>(addr::kIsDemoPlayback)()) return true;
	return (ModRulesActive() & kRuleCheats) != 0;
}

bool KnownVerb(const std::string &verb)
{
	return verb == "gold" || verb == "food" || verb == "map" || verb == "maxpop" || verb == "population" || verb == "heal" || verb == "exph" || verb == "expt" ||
		verb == "peace" || verb == "vision" || verb == "tech" || verb == "level" || verb == "repair" || verb == "ration" || verb == "kill" || verb == "capture" || verb == "spawn" ||
		verb == "sk" || verb == "sh" || verb == "sf" || verb == "sl";
}

int Amount(const std::string &arg, int def)
{
	const int n = arg.empty() ? def : atoi(arg.c_str());
	return std::max(1, std::min(100000, n));
}

std::string Radius(const std::string &arg)
{
	return std::to_string(std::max(100, std::min(6000, atoi(arg.c_str()))));
}

struct Tech {
	const char *name;
	unsigned races;
};
const Tech kTechs[] = {
	{"Fights", 0x1},
	{"Britain Fights", 0x20},
	{"MGladiator Shows", 0x10},
	{"RGladiator Shows", 0x2},
	{"People's Assembly", 0x4},
	{"Tournaments", 0x8},
	{"Gaul Training 1", 0x1},
	{"Rome Training 1", 0x2},
	{"Gaul Training 2", 0x1},
	{"Rome Training 2", 0x2},
	{"Veteran Offence", 0x2},
	{"Veteran Defence", 0x2},
	{"Veteran Health", 0x2},
	{"Gaul Training 3", 0x1},
	{"Shared Strength", 0x4},
	{"Shared Triumph", 0x4},
	{"Heroes' Wisdom", 0x8},
	{"Warriors' Wisdom", 0x8},
	{"Trident Warrior Exp1", 0x1},
	{"Trident Warrior Exp2", 0x1},
	{"Trident Warrior Exp3", 0x1},
	{"Warrior Tales", 0x1},
	{"Shrine of Thor", 0x20},
	{"Mountain Clan", 0x8},
	{"Spoils of war", 0x4},
	{"Liberati guild", 0x10},
	{"Military Academy", 0x10},
	{"Mountain Clan Legacy", 0x8},
	{"Battle tactics", 0x1},
	{"Nobility", 0x8},
	{"BNobility", 0x20},
	{"RNobility", 0x2},
	{"Noble Offence", 0x20},
	{"Noble Defence", 0x20},
	{"Standartization I", 0x10},
	{"Standartization II", 0x10},
	{"Standartization III", 0x10},
	{"Cult of Anubis", 0x40},
	{"Statue of Isis, mother of Anubis", 0x40},
	{"Path of the dead I", 0x40},
	{"Path of the dead II", 0x40},
	{"Path of the dead III", 0x40},
	{"Cult of Horus", 0x40},
	{"Statue of Osiris, father of Horus", 0x40},
	{"Light of Horus I", 0x40},
	{"Light of Horus II", 0x40},
	{"Light of Horus III", 0x40},
	{"Statue of Amon-Ra", 0x40},
	{"TFights", 0x80},
	{"Legendary Valkyries", 0x80},
	{"Learning", 0x80},
	{"Attack focus", 0x80},
	{"Teuton Master", 0x80},
	{"Barrack Level 1", 0xff},
	{"Barrack Level 2", 0xdf},
	{"BBarrack Level 2", 0x20},
	{"Barrack Level 3", 0x4c},
	{"Strenghten Stronghold Defences I", 0xff},
	{"Strenghten Stronghold Defences II", 0xff},
	{"Roman Archers", 0x12},
	{"Roman Velit", 0x10},
	{"Roman Principle", 0x12},
	{"Roman Scout", 0x12},
	{"Roman Praetorian", 0x10},
	{"Roman Full Armor", 0x2},
	{"Roman Chariot Wheels", 0x2},
	{"Gaul Axeman", 0x1},
	{"Gaul Iron Axes", 0x1},
	{"Gaul Spearman", 0x1},
	{"Gaul Iron Spearheads", 0x1},
	{"Gaul Horseman", 0x1},
	{"Gaul Woman Warrior", 0x1},
	{"Gaul Fine Armor", 0x1},
	{"Spears", 0x8},
	{"Horseshoes", 0x8},
	{"Slings", 0x8},
	{"Battleaxes", 0x8},
	{"Forge spears", 0x4},
	{"Forge javelins", 0x4},
	{"Forge swords", 0x4},
	{"Forge maces", 0x4},
	{"Forge pikes", 0x4},
	{"Britain Spears", 0x20},
	{"Britain Large Shields", 0x20},
	{"Britain Javelins", 0x20},
	{"Britain Swords", 0x20},
	{"Britain chamber", 0x20},
	{"Britain Ghoul Summoning", 0x20},
	{"Wolf Summoning", 0x20},
	{"Britain Mass Heal", 0x20},
	{"Dark Rituals", 0x4},
	{"Zombify", 0x4},
	{"Sacred Legion", 0x4},
	{"Ferocity", 0x4},
	{"Ritual Chamber", 0x21},
	{"Ghoul Summoning", 0x21},
	{"Restoration", 0x21},
	{"Eagle Summoning", 0x21},
	{"Cult of Osiris", 0x40},
	{"EZombify", 0x40},
	{"ECloud of Plague", 0x40},
	{"Mumification", 0x40},
	{"GBarrack Level 3", 0x1},
	{"MBarrack Level 3", 0x10},
	{"RBarrack Level 3", 0x2},
	{"Code of Valor", 0x8},
	{"Ancestral Knowledge", 0x8},
	{"Cover of Mercy", 0x8},
	{"Iberian Mass Heal", 0x8},
	{"Free Beer", 0x1},
	{"Free Wine", 0x12},
	{"Free Trade", 0x4},
	{"Free Drinks", 0x8},
	{"Gambling", 0x20},
	{"Gambler's Day", 0x20},
	{"Gambler's Test", 0x20},
	{"Gambler's Luck", 0x20},
	{"Import horses", 0x12},
	{"Food Tax", 0x1},
	{"Nordic trade routes", 0x1},
	{"Belts of Might", 0x1},
	{"Mercenary pact", 0x4},
	{"GrainTrade", 0x40},
	{"ReligiousFeast", 0x40},
	{"TGrainTrade", 0x80},
	{"TFoodTax", 0x80},
	{"TReligiousFeast", 0x80},
	{"TBarrack Level 3", 0x80},
	{"Altar of Jupiter", 0x12},
	{"Cloud of Plague", 0x12},
	{"Wrath of Jupiter", 0x12},
	{"God's Gift", 0x12},
	{"German chamber", 0x80},
	{"German Wolf Summoning", 0x80},
	{"German Eagle Summoning", 0x80},
	{"German Mass Heal", 0x80},
};

struct UnitDef {
	const char *id;
	int civ;
	const char *name;
	bool hero;
};
const UnitDef kUnits[] = {
	{"GArcher", 0, "Archer", false},
	{"GAxeman", 0, "Axeman", false},
	{"GDruid", 0, "Druid", false},
	{"GHero1", 0, "Mraxis", true},
	{"GHero2", 0, "Ogox", true},
	{"GHero3", 0, "Dumnorix", true},
	{"GHero4", 0, "Ercenberht", true},
	{"GHorseman", 0, "Horseman", false},
	{"GSpearman", 0, "Spearman", false},
	{"GSwordsman", 0, "Swordsman", false},
	{"GTridentWarrior", 0, "Trident Warrior", false},
	{"GWomanWarrior", 0, "Woman Warrior", false},
	{"RArcher", 1, "Archer", false},
	{"RChariot", 1, "Chariot", false},
	{"RGladiator", 1, "Gladiator", false},
	{"RHastatus", 1, "Hastatus", false},
	{"MHero1", 1, "Rutilanus", true},
	{"MHero2", 1, "Damasus", true},
	{"MHero3", 1, "Sabucius", true},
	{"RHero1", 1, "Anteros", true},
	{"RHero2", 1, "Chilo", true},
	{"RHero3", 1, "Dioges", true},
	{"RLiberatus", 1, "Liberatus", false},
	{"RPraetorian", 1, "Praetorian", false},
	{"RPriest", 1, "Priest", false},
	{"RPrinciple", 1, "Principle", false},
	{"RScout", 1, "Scout", false},
	{"RTribune", 1, "Tribune", false},
	{"RVelit", 1, "Velit", false},
	{"CBerberAssassin", 2, "Berber Assassin", false},
	{"CHero1", 2, "Mago", true},
	{"CHero2", 2, "Hamack", true},
	{"CHero3", 2, "Carthalo", true},
	{"CJavelinThrower", 2, "Numidian Javelin Thrower", false},
	{"CLibyanFootman", 2, "Libyan Footman", false},
	{"CMaceman", 2, "Maceman", false},
	{"CNoble", 2, "Noble", false},
	{"CNumidianRider", 2, "Numidian Rider", false},
	{"CShaman", 2, "Shaman", false},
	{"CWarElephant", 2, "War Elephant", false},
	{"IArcher", 3, "Archer", false},
	{"ICavalry", 3, "Cavalry", false},
	{"IDefender", 3, "Defender", false},
	{"IEliteGuard", 3, "Elite Guard", false},
	{"IEnchantress", 3, "Enchantress", false},
	{"IHero1", 3, "Tantamo", true},
	{"IHero2", 3, "Caraunio", true},
	{"IHero3", 3, "Baraeco", true},
	{"IMilitiaman", 3, "Militiaman", false},
	{"IMountaineer", 3, "Mountaineer", false},
	{"IScout", 3, "Scout", false},
	{"ISlinger", 3, "Slinger", false},
	{"BBowman", 4, "Bowman", false},
	{"BBronzeSpearman", 4, "Bronze Spearman", false},
	{"BDruid", 4, "Druid", false},
	{"BHero1", 4, "Gawain", true},
	{"BHero2", 4, "Alvin", true},
	{"BHero3", 4, "Wuffa", true},
	{"BHighlander", 4, "Highlander", false},
	{"BJavelineer", 4, "Javelineer", false},
	{"BShieldBearer", 4, "Shield Bearer", false},
	{"BSwordsman", 4, "Swordsman", false},
	{"BVikingLord", 4, "Viking Lord", false},
	{"TArcher", 5, "Archer", false},
	{"TAxeman", 5, "Axeman", false},
	{"TEnchantress", 5, "Enchantress", false},
	{"THero1", 5, "Otho", true},
	{"THero2", 5, "Eidhart", true},
	{"THero3", 5, "Amalrich", true},
	{"THuntress", 5, "Huntress", false},
	{"TMaceman", 5, "Maceman", false},
	{"TSwordsman", 5, "Swordsman", false},
	{"TTeutonArcher", 5, "Teuton Archer", false},
	{"TTeutonRider", 5, "Teuton Rider", false},
	{"TTeutonWolf", 5, "Teuton Wolf", false},
	{"TValkyrie", 5, "Valkyrie", false},
	{"EAnubisWarrior", 6, "Anubis Warrior", false},
	{"EArcher", 6, "Archer", false},
	{"EAxetrower", 6, "Axe Thrower", false},
	{"EChariot", 6, "Chariot of Osiris", false},
	{"EGuardian", 6, "Guardian", false},
	{"EHero1", 6, "Mentuhotep", true},
	{"EHero2", 6, "Nebnufer", true},
	{"EHero3", 6, "Imhotep", true},
	{"EHorusWarrior", 6, "Horus Warrior", false},
	{"EPriest", 6, "Priest", false},
	{"ESwordsman", 6, "Swordsman", false},
	{"Caesar", 7, "Caesar", true},
	{"NHero1", 7, "Heardred", true},
	{"Keltill", 7, "Keltill", true},
	{"Larax", 7, "Larax", true},
	{"DLleldoryn", 7, "Lleldoryn", true},
	{"GHeroWoman", 7, "Morgatha", true},
	{"CHero4", 7, "Unknown, Carthaginian", true},
	{"IHero4", 7, "Unknown, Iberian", true},
};
const int kUnitCount = static_cast<int>(sizeof(kUnits) / sizeof(kUnits[0]));

std::vector<std::string> TechScripts(int player)
{
	std::vector<std::string> out;
	const std::string p = std::to_string(player);
	const std::string declarations = "ObjList ol; Building b; Building cb; Settlement s; int i; ";
	const std::string scan = "ol = ClassPlayerObjs(\"BaseBuilding\", " + p +
		").GetObjList(); for (i = 0; i < ol.count; i += 1) { b = ol[i].AsBuilding(); if (b.IsValid()) { s = b.settlement; if (s.IsValid()) { cb = s.GetCentralBuilding; if (cb.IsValid() && cb.pos.x == b.pos.x && cb.pos.y == b.pos.y";
	for (int race = 0; race < 8; race++) {
		std::string batch;
		int inBatch = 0;
		auto flush = [&]() {
			if (!inBatch) return;
			out.push_back(declarations + scan + " && cb.race == " + std::to_string(race) + ") { " + batch + "} } } }");
			batch.clear();
			inBatch = 0;
		};
		for (const Tech &t : kTechs) {
			if (!(t.races & (1u << race))) continue;
			batch += std::string("b.Research(\"") + t.name + "\"); ";
			if (++inBatch == 8) flush();
		}
		flush();
	}
	out.push_back(declarations + "Mod_Log(\"Researching technologies: player " + p + ", race \" + GetPlayerRace(" + p + ") + \", forums \" + ClassPlayerObjs(\"BaseTownhall\", " + p +
		").count + \", buildings \" + ClassPlayerObjs(\"BaseBuilding\", " + p + ").count); " + scan +
		") { Mod_Log(\"  settlement at \" + b.pos.x + \",\" + b.pos.y + \": race \" + cb.race + \" (\" + cb.name + \") Horseshoes=\" + EnvReadString(s, \"Horseshoes\") + \" RScout=\" + EnvReadString(s, \"RScout\") + \" GHorseman=\" + EnvReadString(s, \"GHorseman\") + \" Arrows=\" + EnvReadString(s, \"Arrows\") + \" RArcher=\" + EnvReadString(s, \"RArcher\")); } } } }");
	out.push_back("ObjList u; int i; InvalidateDamageFormulaParams(); InvalidateRegenConsts(); u = GetPlayerUnits(" + p +
		", \"Military\"); for (i = 0; i < u.count; i += 1) { if (u[i].IsValid()) u[i].AsUnit().RecalcBonuses(); }");
	return out;
}

std::string Snippet(const std::string &verb, const std::string &arg, int player, const std::string &ref, std::string &label)
{
	const std::string p = std::to_string(player);
	const std::string refx = std::to_string(atoi(ref.c_str()));
	const size_t refComma = ref.find(',');
	const std::string refy = std::to_string(refComma == std::string::npos ? 0 : atoi(ref.c_str() + refComma + 1));
	if (verb == "gold" || verb == "food") {
		const bool gold = verb == "gold";
		const std::string n = std::to_string(Amount(arg, 1000));
		label = std::string(gold ? "gold " : "food ") + n;
		return "Query q; Obj b; q = ClassPlayerObjs(\"BaseBuilding\", " + p + "); if (q.count > 0) { b = q.NearestObj(Point(" + ref + ")); b.AsBuilding.settlement." +
			(gold ? "SetGold(b.AsBuilding.settlement.gold + " : "SetFood(b.AsBuilding.settlement.food + ") + n + "); }";
	}
	if (verb == "maxpop") {
		const std::string n = std::to_string(Amount(arg, 50));
		label = "max population " + n;
		return "Query q; Obj b; q = ClassPlayerObjs(\"BaseBuilding\", " + p + "); if (q.count > 0) { b = q.NearestObj(Point(" + ref + ")); b.AsBuilding.settlement.AddToMaxPopulation(" + n + "); }";
	}
	if (verb == "population") {
		const std::string n = std::to_string(Amount(arg, 100));
		label = "population " + n;
		return "Query q; Obj b; q = ClassPlayerObjs(\"BaseBuilding\", " + p + "); if (q.count > 0) { b = q.NearestObj(Point(" + ref + ")); b.AsBuilding.settlement.SetPopulation(b.AsBuilding.settlement.population + " + n + "); }";
	}
	if (verb == "exph" || verb == "expt") {
		const bool heroes = verb == "exph";
		const std::string n = std::to_string(Amount(arg, 1000));
		label = std::string("experience ") + n + (heroes ? " to each hero" : " to each troop");
		return std::string("Query q; q = ClassPlayerObjs(\"") + (heroes ? "Hero" : "Military") + "\", " + p + "); if (q.count > 0) { DistributeExperience(q, q.count * " + n + "); }";
	}
	if (verb == "peace" || verb == "vision") {
		const bool peace = verb == "peace";
		label = peace ? "ceasefire with everyone" : "vision of everyone";
		const std::string each = peace ? "DiplCeaseFire(" + p + ", i, true); DiplCeaseFire(i, " + p + ", true);" : "DiplShareView(i, " + p + ", true);";
		return "int i; for (i = 1; i <= 8; i += 1) { if (i != " + p + " && ClassPlayerObjs(\"BaseTownhall\", i).count > 0) { " + each + " } }";
	}
	if (verb == "level") {
		const std::string n = std::to_string(std::max(1, std::min(999, atoi(arg.c_str()))));
		label = "heroes and troops to level " + n;
		return "ObjList ol; ObjList ml; int i; ol = GetPlayerUnits(" + p + ", \"Hero\"); ml = GetPlayerUnits(" + p + ", \"Military\"); ol.AddList(ml); for (i = 0; i < ol.count; i += 1) { if (ol[i].IsValid()) { ol[i].AsUnit().SetLevel(" + n + "); } }";
	}
	if (verb == "repair") {
		label = "building repaired";
		return "Query q; Obj o; Building b; q = ClassPlayerObjs(\"BaseBuilding\", " + p + "); if (q.count > 0) { o = q.NearestObj(Point(" + ref + ")); b = o.AsBuilding(); if (b.IsValid()) { b.RRepair(); } }";
	}
	if (verb == "ration") {
		label = "troops fed";
		return "ObjList ol; int i; ol = Intersect(ObjsInCircle(Point(" + ref + "), " + Radius(arg) + ", \"Military\"), ClassPlayerObjs(\"Military\", " + p +
			")).GetObjList(); for (i = 0; i < ol.count; i += 1) { if (ol[i].IsValid()) { ol[i].AsUnit().SetFood(ol[i].AsUnit().maxfood); } }";
	}
	if (verb == "kill") {
		label = "unit removed";
		return "ObjList ol; point b; int i; int d; int bd; int idx; int found; bd = 90000; found = 0; idx = 0; ol = ObjsInCircle(Point(" + ref + "), 300, \"Unit\").GetObjList(); for (i = 0; i < ol.count; i += 1) { if (ol[i].IsValid()) { b = ol[i].pos; d = (b.x - " + refx + ") * (b.x - " + refx + ") + (b.y - " + refy + ") * (b.y - " + refy + "); if (d < bd) { bd = d; idx = i; found = 1; } } } if (found == 1) { ol[idx].Damage(100000); }";
	}
	if (verb == "capture") {
		label = "settlement captured";
		return "ObjList ol; Building bd; point b; int i; int o; int d; int best; int idx; int found; best = 360000; found = 0; idx = 0; ol = ObjsInCircle(Point(" + ref + "), 600, \"Building\").GetObjList(); for (i = 0; i < ol.count; i += 1) { if (ol[i].IsValid()) { o = ol[i].player; if (o != " + p +
			" && (o < 1 || o > 8 || !DiplAreAllied(o, " + p + "))) { b = ol[i].pos; d = (b.x - " + refx + ") * (b.x - " + refx + ") + (b.y - " + refy + ") * (b.y - " + refy + "); if (d < best) { best = d; idx = i; found = 1; } } } } if (found == 1) { bd = ol[idx].AsBuilding(); bd.settlement.SetPlayer(" + p + "); }";
	}
	if (verb == "spawn") {
		const size_t comma = arg.find(',');
		const int idx = atoi(arg.c_str());
		const int n = comma == std::string::npos ? 1 : std::max(1, std::min(kCheatsMaxSpawn, atoi(arg.c_str() + comma + 1)));
		if (idx < 0 || idx >= kUnitCount) return std::string();
		label = std::string(kUnits[idx].name) + " x" + std::to_string(n);
		return "Query q; Obj b; Unit u; point e; int k; q = ClassPlayerObjs(\"BaseBuilding\", " + p + "); if (q.count > 0) { b = q.NearestObj(Point(" + ref + ")); e = b.AsBuilding.GetExitPoint(b.pos, false); for (k = 0; k < " +
			std::to_string(n) + "; k += 1) { u = Place(\"" + kUnits[idx].id + "\", e, " + p + "); b.AsBuilding.settlement.ForceAddUnit(u); } }";
	}
	if (verb == "heal") {
		label = "troops healed";
		return "Query q; q = Intersect(ObjsInCircle(Point(" + ref + "), " + Radius(arg) + ", \"Military\"), ClassPlayerObjs(\"Military\", " + p + ")); q.Heal(100000); q.HealStamina(100000);";
	}
	if (verb == "map") {
		label = "map explored";
		return "point m; int x; int y; m = GetMapSize(); for (x = 0; x <= m.x; x += 3500) { for (y = 0; y <= m.y; y += 3500) { ExploreCircle(" + p + ", Point(x, y), 4500); } }";
	}
	return std::string();
}

bool Readable(const void *p, size_t n)
{
	MEMORY_BASIC_INFORMATION mbi;
	if (!p || reinterpret_cast<uintptr_t>(p) < 0x10000 || !VirtualQuery(p, &mbi, sizeof(mbi))) return false;
	if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
	return reinterpret_cast<const char *>(p) + n <= static_cast<const char *>(mbi.BaseAddress) + mbi.RegionSize;
}

struct Selection {
	int count = 0, kind = 0;
	int fx = 0, fy = 0;
	int cx = 0, cy = 0;
	int radius = 0;
	std::vector<unsigned short> handles;
};

bool ReadSelection(Selection &s)
{
	char *sel = *reinterpret_cast<char **>(addr::kSelectionManager);
	if (!Readable(sel + 0xf0, 0x20)) return false;
	const int count = *reinterpret_cast<int *>(sel + 0xf0 + 0x1c);
	if (count <= 0 || count > 100000) return false;
	char *head = *reinterpret_cast<char **>(sel + 0xf0 + 0x18);
	s.count = count;
	s.handles.clear();
	s.kind = *reinterpret_cast<int *>(sel + 0xf8);
	int xs[300], ys[300], n = 0;
	if (!Readable(head, 8)) return false;
	char *node = *reinterpret_cast<char **>(head);
	for (int guard = 0; node && node != head && n < 300 && guard < count + 8; guard++) {
		if (!Readable(node, 12)) break;
		const unsigned short handle = *reinterpret_cast<unsigned short *>(node + 8);
		char *obj = reinterpret_cast<char **>(addr::kHandleTable)[handle];
		if (Readable(obj + 0x24, 8)) {
			const int ox = *reinterpret_cast<int *>(obj + 0x24), oy = *reinterpret_cast<int *>(obj + 0x28);
			if (ox >= 0 && oy >= 0 && ox < 4000000 && oy < 4000000) {
				xs[n] = ox;
				ys[n] = oy;
				s.handles.push_back(handle);
				n++;
			}
		}
		node = *reinterpret_cast<char **>(node);
	}
	if (n == 0) return false;
	s.fx = xs[0];
	s.fy = ys[0];
	long long sx = 0, sy = 0;
	for (int i = 0; i < n; i++) {
		sx += xs[i];
		sy += ys[i];
	}
	s.cx = static_cast<int>(sx / n);
	s.cy = static_cast<int>(sy / n);
	int farthest = 0;
	for (int i = 0; i < n; i++) {
		const int dx = xs[i] - s.cx, dy = ys[i] - s.cy;
		farthest = std::max(farthest, static_cast<int>(std::sqrt(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy)));
	}
	s.radius = std::max(200, std::min(6000, farthest + 150));
	return true;
}

bool ReferencePoint(int &x, int &y)
{
	Selection s;
	if (ReadSelection(s)) {
		x = s.fx;
		y = s.fy;
		return true;
	}
	char *view = *reinterpret_cast<char **>(addr::kVisibleMgr);
	if (Readable(view + 0x618, 0x10)) {
		unsigned in[2] = {static_cast<unsigned>((*reinterpret_cast<int *>(view + 0x618) + *reinterpret_cast<int *>(view + 0x620)) / 2),
			static_cast<unsigned>((*reinterpret_cast<int *>(view + 0x61c) + *reinterpret_cast<int *>(view + 0x624)) / 2)};
		unsigned out[2] = {};
		reinterpret_cast<void(__stdcall *)(unsigned *, unsigned *)>(addr::kScreenToMap)(out, in);
		if (static_cast<int>(out[0]) >= 0 && static_cast<int>(out[1]) >= 0 && out[0] < 4000000 && out[1] < 4000000) {
			x = static_cast<int>(out[0]);
			y = static_cast<int>(out[1]);
			return true;
		}
	}
	return false;
}

std::string WithRef(const std::string &text, int x, int y) { return text + ":" + std::to_string(x) + "," + std::to_string(y); }

std::string WithReference(const std::string &text)
{
	int x = 0, y = 0;
	if (!ReferencePoint(x, y)) return text;
	return text + ":" + std::to_string(x) + "," + std::to_string(y);
}

const char kAlpha[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-";
const size_t kHandlesPerCommand = 15;
std::deque<std::string> g_cheatQueue;
DWORD g_cheatLastSend = 0;

int AlphaIndex(char c)
{
	const void *p = c ? memchr(kAlpha, c, 64) : nullptr;
	return p ? static_cast<int>(static_cast<const char *>(p) - kAlpha) : -1;
}

void EncodeHandle(std::string &out, unsigned h)
{
	out += kAlpha[(h >> 12) & 63];
	out += kAlpha[(h >> 6) & 63];
	out += kAlpha[h & 63];
}

void QueueSelected(char letter, const std::string &arg, const std::vector<unsigned short> &handles)
{
	for (size_t i = 0; i < handles.size(); i += kHandlesPerCommand) {
		std::string enc;
		for (size_t j = i; j < handles.size() && j < i + kHandlesPerCommand; j++) EncodeHandle(enc, handles[j]);
		g_cheatQueue.push_back(std::string(kPrefix) + "s" + letter + ":" + arg + ":" + enc);
	}
}

void AddHeroArmy(char *obj, std::vector<unsigned> &out)
{
	if (!Readable(obj + 0x1d8, 0x10) || (*reinterpret_cast<unsigned *>(obj + 0x2c) & 0x1000000u) == 0) return;
	char **map = *reinterpret_cast<char ***>(obj + 0x1d8);
	const unsigned mapSize = *reinterpret_cast<unsigned *>(obj + 0x1dc), start = *reinterpret_cast<unsigned *>(obj + 0x1e0), count = *reinterpret_cast<unsigned *>(obj + 0x1e4);
	if (!map || mapSize == 0 || mapSize > 4096 || count == 0 || count > 500) return;
	for (unsigned i = start; i != start + count; i++) {
		unsigned block = i >> 3;
		if (block >= mapSize) block -= mapSize;
		if (!Readable(map + block, 4) || !Readable(map[block] + (i & 7) * 2, 2)) continue;
		out.push_back(*reinterpret_cast<unsigned short *>(map[block] + (i & 7) * 2));
	}
}

std::vector<std::pair<int, int>> PositionsOfHandles(const std::string &enc)
{
	std::vector<unsigned> handles;
	for (size_t i = 0; i + 2 < enc.size(); i += 3) {
		const int a = AlphaIndex(enc[i]), b = AlphaIndex(enc[i + 1]), c = AlphaIndex(enc[i + 2]);
		if (a < 0 || b < 0 || c < 0) continue;
		const unsigned h = ((static_cast<unsigned>(a) << 12) | (static_cast<unsigned>(b) << 6) | static_cast<unsigned>(c)) & 0xffff;
		handles.push_back(h);
		AddHeroArmy(reinterpret_cast<char **>(addr::kHandleTable)[h], handles);
	}
	std::set<unsigned> seen;
	std::vector<std::pair<int, int>> out;
	for (const unsigned h : handles) {
		if (!seen.insert(h).second) continue;
		char *obj = reinterpret_cast<char **>(addr::kHandleTable)[h & 0xffff];
		if (!Readable(obj + 0x24, 8)) continue;
		const int x = *reinterpret_cast<int *>(obj + 0x24), y = *reinterpret_cast<int *>(obj + 0x28);
		if (x < 0 || y < 0 || x >= 4000000 || y >= 4000000) continue;
		out.emplace_back(x, y);
	}
	return out;
}

void RunSelectedScript(char letter, int level, int player, const std::vector<std::pair<int, int>> &positions)
{
	void *game = Game();
	if (!game || positions.empty()) return;
	const std::string p = std::to_string(player), n = std::to_string(level);
	const std::string head = letter == 'k' || letter == 'h' ? "Query q; " : "ObjList ol; int k; ";
	std::string body;
	auto flush = [&]() {
		if (body.empty()) return;
		reinterpret_cast<RunScript_t>(Vt(game)[0xa8 / 4])(game, nullptr, (head + body).c_str());
		body.clear();
	};
	const char *const classes[2] = {"Military", "Hero"};
	for (const std::pair<int, int> &pt : positions) {
		const std::string at = "Point(" + std::to_string(pt.first) + ", " + std::to_string(pt.second) + ")";
		if (letter == 'k') {
			body += "q = ObjsInCircle(" + at + ", 6, \"Unit\"); q.Damage(100000); ";
		}
		else {
			for (const char *cls : classes) {
				const std::string own = std::string("Intersect(ObjsInCircle(") + at + ", 6, \"" + cls + "\"), ClassPlayerObjs(\"" + cls + "\", " + p + "))";
				if (letter == 'h') {
					body += "q = " + own + "; q.Heal(100000); q.HealStamina(100000); ";
				}
				else {
					const std::string apply = letter == 'f' ? "ol[k].AsUnit().SetFood(ol[k].AsUnit().maxfood);" : "ol[k].AsUnit().SetLevel(" + n + ");";
					body += "ol = " + own + ".GetObjList(); for (k = 0; k < ol.count; k += 1) { if (ol[k].IsValid()) { " + apply + " } } ";
				}
			}
		}
		if (body.size() > 1400) flush();
	}
	flush();
}

bool QueueCheat(const std::string &text)
{
	char *game = static_cast<char *>(Game());
	void *player = game ? *reinterpret_cast<void **>(game + 0x12c8) : nullptr;
	if (!player) return false;
	void *cmd = reinterpret_cast<Create_t>(addr::kCmdMessageCreate)();
	if (!cmd) return false;
	reinterpret_cast<Assign_t>(addr::kStringAssign)(static_cast<char *>(cmd) + 0xc, nullptr, text.c_str(), static_cast<unsigned>(text.size()));
	reinterpret_cast<ThisPtr_t>(Vt(cmd)[3])(cmd, nullptr, player);
	reinterpret_cast<ThisPtr_t>(addr::kQueueCommand)(game, nullptr, cmd);
	return true;
}

bool HandleChat(const char *line)
{
	std::string text = line;
	while (!text.empty() && text.back() == ' ') text.pop_back();
	const size_t sp = text.find(' ');
	std::string verb = text.substr(0, sp), arg = sp == std::string::npos ? std::string() : text.substr(sp + 1);
	std::transform(verb.begin(), verb.end(), verb.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
	verb = LegacyCheatVerb(verb);
	if (!KnownVerb(verb)) return false;
	if (!Allowed()) {
		UiToast("Cheats are not allowed in this game. The host can enable them in the room options.");
		return true;
	}
	const std::string wire = verb == "map" ? std::string() : std::to_string(Amount(arg, 1000));
	if (!QueueCheat(WithReference(std::string(kPrefix) + verb + ":" + wire))) UiToast("The cheat could not be sent.", true);
	return true;
}

void __fastcall HookChatSend(void *dlg)
{
	char buf[512] = {};
	void *ctrl = reinterpret_cast<GetChild_t>(Vt(dlg)[0x7c / 4])(dlg, nullptr, 0x1004, *reinterpret_cast<const char **>(addr::kClassNameCUITextW));
	if (ctrl && reinterpret_cast<GetText_t>(addr::kGetTextUtf8)(ctrl, nullptr, buf, sizeof(buf)) == 0 && buf[0] == '/' && HandleChat(buf + 1)) return;
	g_origChatSend(dlg);
}

std::atomic<bool> g_panelOpen{false};
std::atomic<bool> g_panelKeyOpened{false};

}

bool CheatsPanelAvailable()
{
	char *game = static_cast<char *>(Game());
	return game && *reinterpret_cast<unsigned *>(game + 0x1258) > 0 && !GameRoomActive() && !reinterpret_cast<IsDemo_t>(addr::kIsDemoPlayback)() && Allowed();
}

void LocalizeUnits();
bool CheatsPanelOpen() { return g_panelOpen; }
void CheatsPanelToggle()
{
	g_panelOpen = !g_panelOpen;
	if (g_panelOpen) LocalizeUnits();
	else g_panelKeyOpened = false;
}

bool CheatsPanelConsumeKeyOpen() { return g_panelKeyOpened.exchange(false); }

typedef char *(__cdecl *LocFormat_t)(void *db, char *out, const char *key, ...);
std::vector<std::string> g_unitLoc;

bool LocalizeKey(void *db, const char *key, char *out, size_t size)
{
	out[0] = 0;
	__try {
		char buf[512] = {};
		reinterpret_cast<LocFormat_t>(addr::kLocFormat)(db, buf, key);
		strncpy_s(out, size, buf, _TRUNCATE);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

void LocalizeUnits()
{
	if (!g_unitLoc.empty()) return;
	void *db = *reinterpret_cast<void **>(addr::kUiLocalization);
	g_unitLoc.assign(kUnitCount, std::string());
	if (!db) return;
	for (int i = 0; i < kUnitCount; i++) {
		char out[512];
		const std::string withContext = std::string(kUnits[i].name) + "@" + kUnits[i].id + " class name";
		if (LocalizeKey(db, withContext.c_str(), out, sizeof(out)) && out[0] && withContext != out) g_unitLoc[i] = out;
		else if (LocalizeKey(db, kUnits[i].name, out, sizeof(out)) && out[0] && strcmp(out, kUnits[i].name) != 0) g_unitLoc[i] = out;
	}
}

int CheatsUnitCount() { return kUnitCount; }
int CheatsUnitCiv(int i) { return i >= 0 && i < kUnitCount ? kUnits[i].civ : 0; }
const char *CheatsUnitName(int i)
{
	if (i < 0 || i >= kUnitCount) return "";
	return static_cast<int>(g_unitLoc.size()) == kUnitCount && !g_unitLoc[i].empty() ? g_unitLoc[i].c_str() : kUnits[i].name;
}
bool CheatsUnitIsHero(int i) { return i >= 0 && i < kUnitCount && kUnits[i].hero; }

void CheatsSpawn(int index, int count)
{
	if (!Allowed()) {
		UiToast("Cheats are not allowed in this game. The host can enable them in the room options.");
		return;
	}
	if (index < 0 || index >= kUnitCount) return;
	Selection s;
	if (!ReadSelection(s)) {
		UiToast("Select a forum or a fort first.");
		return;
	}
	const std::string wire = std::to_string(index) + "," + std::to_string(std::max(1, std::min(kCheatsMaxSpawn, count)));
	if (!QueueCheat(WithRef(std::string(kPrefix) + "spawn:" + wire, s.fx, s.fy))) UiToast("The cheat could not be sent.", true);
}

void CheatsRun(int verb, int amount)
{
	if (!Allowed()) {
		UiToast("Cheats are not allowed in this game. The host can enable them in the room options.");
		return;
	}
	static const char *const kVerbs[] = {"gold", "food", "map", "maxpop", "heal", "exph", "expt", "peace", "vision", "tech", "level", "repair", "ration", "kill", "capture", "population"};
	if (verb < 0 || verb > 15) return;
	Selection s;
	const bool has = ReadSelection(s);
	const bool building = has && s.kind >= 4;
	if (has && !building && !s.handles.empty() && (verb == 4 || verb == 10 || verb == 12 || verb == 13)) {
		const char letter = verb == 4 ? 'h' : verb == 12 ? 'f' : verb == 13 ? 'k' : 'l';
		QueueSelected(letter, verb == 10 ? std::to_string(std::max(1, std::min(999, amount))) : std::string(), s.handles);
		return;
	}
	std::string wire;
	int rx = s.fx, ry = s.fy;
	switch (verb) {
	case 0: case 1: case 3: case 15:
		if (!has) { UiToast("Select a forum first: the resource goes to its settlement."); return; }
		wire = std::to_string(std::max(1, std::min(100000, amount)));
		break;
	case 11:
		if (!has) { UiToast("Select the building you want to repair first."); return; }
		break;
	case 4: case 12:
		if (!has) { UiToast("Select the troops first."); return; }
		wire = std::to_string(building ? 300 : s.radius);
		rx = building ? s.fx : s.cx;
		ry = building ? s.fy : s.cy;
		break;
	case 10:
		wire = std::to_string(std::max(1, std::min(999, amount))) + "," + std::to_string(has && !building ? s.radius : 0);
		rx = s.cx;
		ry = s.cy;
		break;
	case 13:
		UiToast("Select the troops first.");
		return;
	case 14:
		if (!has) { UiToast("Select a building of the settlement you want to capture first."); return; }
		break;
	default:
		break;
	}
	std::string text = std::string(kPrefix) + kVerbs[verb] + ":" + wire;
	if (has) text = WithRef(text, rx, ry);
	else text = WithReference(text);
	if (!QueueCheat(text)) UiToast("The cheat could not be sent.", true);
}

void CheatsFrameTick()
{
	if (g_cheatQueue.empty()) return;
	const DWORD now = GetTickCount();
	if (now - g_cheatLastSend < 130) return;
	g_cheatLastSend = now;
	const std::string text = g_cheatQueue.front();
	g_cheatQueue.pop_front();
	if (!QueueCheat(text)) g_cheatQueue.clear();
}

bool CheatsHandleKey(unsigned msg, unsigned wp, long)
{
	if ((msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN) || !CheatsPanelAvailable()) return false;
	if (!ControlsModKeyPressed(kModKeyCheats, wp)) return false;
	CheatsPanelToggle();
	g_panelKeyOpened = g_panelOpen.load();
	return true;
}

bool CheatsHandleCommand(void *cmd, const char *text, unsigned size)
{
	if (!text || size <= kPrefixLen || size > 64 || memcmp(text, kPrefix, kPrefixLen) != 0) return false;
	const std::string body(text + kPrefixLen, size - kPrefixLen);
	const size_t colon = body.find(':');
	const std::string verb = body.substr(0, colon);
	std::string arg = colon == std::string::npos ? std::string() : body.substr(colon + 1);
	const std::string rawArg = arg;
	std::string ref = "0, 0";
	if (const size_t c2 = arg.find(':'); c2 != std::string::npos) {
		const std::string pt = arg.substr(c2 + 1);
		arg = arg.substr(0, c2);
		const long x = atol(pt.c_str());
		const size_t comma = pt.find(',');
		const long y = comma == std::string::npos ? 0 : atol(pt.c_str() + comma + 1);
		if (x >= 0 && x < 4000000 && y >= 0 && y < 4000000) ref = std::to_string(x) + ", " + std::to_string(y);
	}
	char *who = cmd ? *reinterpret_cast<char **>(static_cast<char *>(cmd) + 4) : nullptr;
	const int slot = who ? *reinterpret_cast<int *>(who + 8) : -1;
	if (slot < 0 || slot > 7 || !Game()) {
		LogW("cheats", "command without a valid player; ignored");
		return true;
	}
	if (!KnownVerb(verb)) {
		LogW("cheats", "unknown command from player %d; ignored", slot + 1);
		return true;
	}
	if (!Allowed()) {
		LogW("cheats", "command \"%s\" from player %d ignored (the game does not allow cheats)", verb.c_str(), slot + 1);
		return true;
	}
	void *game = Game();
	if (verb.size() == 2 && verb[0] == 's') {
		const size_t sep = rawArg.find(':');
		const int level = std::max(1, std::min(999, atoi(rawArg.c_str())));
		const std::vector<std::pair<int, int>> positions = PositionsOfHandles(sep == std::string::npos ? std::string() : rawArg.substr(sep + 1));
		LogI("cheats", "player %d uses \"%s\" on %u selected units", slot + 1, verb.c_str(), static_cast<unsigned>(positions.size()));
		RunSelectedScript(verb[1], level, slot + 1, positions);
		return true;
	}
	if (verb == "tech") {
		const std::vector<std::string> scripts = TechScripts(slot + 1);
		LogI("cheats", "player %d researches every technology (%u script fragments)", slot + 1, static_cast<unsigned>(scripts.size()));
		for (const std::string &code : scripts) reinterpret_cast<RunScript_t>(Vt(game)[0xa8 / 4])(game, nullptr, code.c_str());
		return true;
	}
	std::string label;
	const std::string code = Snippet(verb, arg, slot + 1, ref, label);
	LogI("cheats", "player %d uses \"%s\": %s", slot + 1, label.c_str(), code.c_str());
	reinterpret_cast<RunScript_t>(Vt(game)[0xa8 / 4])(game, nullptr, code.c_str());
	return true;
}

void InstallCheats(bool knownExe)
{
	if (!knownExe) return;
	const bool ok = HookCreate(reinterpret_cast<void *>(addr::kChatSend), reinterpret_cast<void *>(&HookChatSend), reinterpret_cast<void **>(&g_origChatSend)) == MH_OK &&
		HookEnable(reinterpret_cast<void *>(addr::kChatSend)) == MH_OK;
	LogAt(ok ? kLogInfo : kLogError, "cheats", "chat commands: %s", ok ? "ready (/gold, /food and /map in the chat)" : "NOT available");
}
