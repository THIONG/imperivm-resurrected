// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>
#include <vector>

#include "core/json.h"
#include "mods/gbref.h"

namespace {

void ModsKeepTheirPlainNumber()
{
	CHECK_EQ(GbIsSound(726223), false);
	CHECK_EQ(GbRow(726223), 726223ull);
	CHECK_EQ(std::string(GbModel(726223)), std::string("Mod"));
	CHECK_EQ(GbApiPath(726223), std::string("/Mod/726223"));
	CHECK_EQ(GbCatalogId(726223), std::string("gamebanana-726223"));
	CHECK_EQ(GbKeyText(726223), std::string("726223"));
}

void SoundsAreTheirOwnSection()
{
	const uint64_t key = GbSoundKey(86204);
	CHECK_EQ(GbIsSound(key), true);
	CHECK_EQ(GbRow(key), 86204ull);
	CHECK(key != 86204ull);
	CHECK_EQ(GbApiPath(key), std::string("/Sound/86204"));
	CHECK_EQ(GbCatalogId(key), std::string("gamebanana-sound-86204"));
	CHECK_EQ(GbKeyText(key), std::string("sound 86204"));
	CHECK_EQ(std::to_string(key), std::to_string(static_cast<uint64_t>(static_cast<double>(key))));
}

void KeysFromProfileUrls()
{
	CHECK_EQ(GbKeyFromUrl("https://gamebanana.com/mods/726223"), 726223ull);
	CHECK_EQ(GbKeyFromUrl("https://GameBanana.com/Sounds/86204"), GbSoundKey(86204));
	CHECK_EQ(GbKeyFromUrl("https://gamebanana.com/mods/cats/50093"), 0ull);
	CHECK_EQ(GbKeyFromUrl("https://gamebanana.com/games/26069"), 0ull);
	CHECK_EQ(GbKeyFromUrl("https://example.com/mods/1"), 0ull);
	CHECK_EQ(GbKeyFromUrl("https://gamebanana.com/mods/99999999999999"), 0ull);
}

void PicturesInBothApiShapes()
{
	Json v11;
	CHECK(Json::Parse("{\"_aPreviewMedia\":{\"_aImages\":["
		"{\"_sType\":\"screenshot\",\"_sBaseUrl\":\"https://img/ss\",\"_sFile\":\"a.jpg\",\"_sFile220\":\"220-a.jpg\",\"_sFile530\":\"530-a.jpg\"},"
		"{\"_sType\":\"screenshot\",\"_sBaseUrl\":\"https://img/ss\",\"_sFile\":\"b.jpg\"}]}}", v11));
	std::vector<GbPicture> p = GbPictures(v11, false);
	CHECK_EQ(p.size(), static_cast<size_t>(2));
	CHECK_EQ(p[0].big, std::string("https://img/ss/a.jpg"));
	CHECK_EQ(p[0].card, std::string("https://img/ss/530-a.jpg"));
	CHECK_EQ(p[0].thumb, std::string("https://img/ss/220-a.jpg"));
	CHECK_EQ(p[1].card, std::string("https://img/ss/b.jpg"));
	CHECK_EQ(p[1].thumb, std::string("https://img/ss/b.jpg"));

	Json profile13;
	CHECK(Json::Parse("{\"_aPreviewContent\":{\"screenshots\":["
		"{\"_sBaseUrl\":\"https://img/ss\",\"_sFile\":\"c.jpg\",\"_sFile530\":\"c_530.webp\",\"_sFile220\":\"c_220.webp\"},"
		"{\"_sBaseUrl\":\"https://img/ss\",\"_sFile\":\"d.jpg\"}]}}", profile13));
	p = GbPictures(profile13, true);
	CHECK_EQ(p.size(), static_cast<size_t>(2));
	CHECK_EQ(p[0].card, std::string("https://img/ss/c_530.webp"));
	CHECK_EQ(p[0].thumb, std::string("https://img/ss/c_220.webp"));
	CHECK_EQ(p[1].big, std::string("https://img/ss/d.jpg"));
	p = GbPictures(profile13, false);
	CHECK_EQ(p[0].card, std::string("https://img/ss/c.jpg"));
	CHECK_EQ(p[0].thumb, std::string("https://img/ss/c.jpg"));
	CHECK_EQ(p[0].big, std::string("https://img/ss/c.jpg"));

	Json index13;
	CHECK(Json::Parse("{\"_aPreviewContent\":{\"screenshot\":{\"_sBaseUrl\":\"https://img/ss\",\"_sFile\":\"e.jpg\",\"_sFile220\":\"220-e.jpg\"}}}", index13));
	p = GbPictures(index13, true);
	CHECK_EQ(p.size(), static_cast<size_t>(1));
	CHECK_EQ(p[0].thumb, std::string("https://img/ss/220-e.jpg"));

	Json sound13;
	CHECK(Json::Parse("{\"_aPreviewContent\":{\"audio\":{\"_sUrl\":\"https://x/a.mp3\"}}}", sound13));
	CHECK(GbPictures(sound13, true).empty());
	CHECK(GbPictures(Json(), true).empty());
}

void SourcesRoundTrip()
{
	Json o = Json::MakeObject();
	GbKeyToJson(o, 726223);
	CHECK_EQ(static_cast<uint64_t>(o.GetNumber("gamebanana")), 726223ull);
	CHECK_EQ(GbKeyFromJson(o), 726223ull);

	GbKeyToJson(o, GbSoundKey(86204));
	CHECK(o.Get("gamebanana") == nullptr);
	CHECK_EQ(static_cast<uint64_t>(o.GetNumber("gamebananaSound")), 86204ull);
	CHECK_EQ(GbKeyFromJson(o), GbSoundKey(86204));

	Json old;
	CHECK(Json::Parse("{\"page\":\"https://gamebanana.com/mods/1\",\"gamebanana\":123}", old));
	CHECK_EQ(GbKeyFromJson(old), 123ull);

	Json bad;
	CHECK(Json::Parse("{\"gamebanana\":-4,\"gamebananaSound\":2e15}", bad));
	CHECK_EQ(GbKeyFromJson(bad), 0ull);

	Json none = Json::MakeObject();
	GbKeyToJson(none, 0);
	CHECK(none.obj.empty());
}

}

int main()
{
	ModsKeepTheirPlainNumber();
	SoundsAreTheirOwnSection();
	KeysFromProfileUrls();
	PicturesInBothApiShapes();
	SourcesRoundTrip();
	return test::Summary("gbref");
}
