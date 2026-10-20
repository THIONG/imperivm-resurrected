// Announces a published release in the Discord channels of new versions: one message per language, sent with that channel's webhook.
// The message points to the website, not to GitHub, and tells players who already have the mod that the game offers the update when
// it starts. If the release notes have a heading per language (## Español, ## English, ## Italiano), each channel also gets its part.
//   node tools/announce-release.js --tag v0.32.11 [--notes notes.md] [--dry-run]
//   --notes    read the notes from a file instead of the release (gh release view, needs GH_TOKEN)
//   --dry-run  print the messages instead of sending them
// Webhooks: DISCORD_WEBHOOK_ES, DISCORD_WEBHOOK_EN, DISCORD_WEBHOOK_IT; a language without one is skipped, and with none it does nothing.
// Roles to mention (optional, role ids): DISCORD_ROLE_ES, DISCORD_ROLE_EN, DISCORD_ROLE_IT. Pre-releases are not announced.
// The release workflow runs it after verifying the release.
'use strict';
const fs = require('fs');
const { execFileSync } = require('child_process');

const args = process.argv.slice(2);
const option = name => { const i = args.indexOf(name); return i >= 0 ? args[i + 1] : ''; };
const tag = option('--tag') || process.env.TAG || '';
const notesFile = option('--notes');
const dryRun = args.includes('--dry-run');
if (!tag) { console.error('usage: node tools/announce-release.js --tag vX.Y.Z [--notes file] [--dry-run]'); process.exit(2); }

const SITE = 'https://www.imperivmresurrected.com';
const RED = 0xc4161c;
const NOTES_LIMIT = 1500;

const LANGUAGES = {
  es: {
    site: SITE + '/',
    heading: /^(español|espanol|castellano|spanish)$/i,
    title: v => `🆕 Imperivm Resurrected ${v}`,
    installed: ['**¿Ya tienes el mod instalado?**', 'Entra al juego y te aparecerá el aviso para actualizarlo.'],
    fresh: site => ['**¿Aún no lo tienes?**', `Descárgalo desde la web: **[imperivmresurrected.com](${site})**`],
    news: '**📋 NOVEDADES**',
    firstOld: ['**¿Vienes de la 0.9.0 o anterior?**', 'Descomprime el zip a mano sobre la carpeta del juego, una sola vez: el aviso del juego no puede instalar esta versión.'],
    firstNews: '**📋 PRIMERA VERSIÓN**',
    more: site => `Más en la [web](${site}).`,
  },
  en: {
    site: SITE + '/en/',
    heading: /^(english|inglés|ingles)$/i,
    title: v => `🆕 Imperivm Resurrected ${v}`,
    installed: ['**Already have the mod installed?**', 'Open the game and you\'ll get the prompt to update it.'],
    fresh: site => ['**Don\'t have it yet?**', `Download it from the website: **[imperivmresurrected.com](${site})**`],
    news: '**📋 WHAT\'S NEW**',
    firstOld: ['**Coming from 0.9.0 or earlier?**', 'Unzip it by hand over the game folder, once: the game\'s own update cannot install this version.'],
    firstNews: '**📋 FIRST RELEASE**',
    more: site => `More on the [website](${site}).`,
  },
  it: {
    site: SITE + '/it/',
    heading: /^(italiano|italian)$/i,
    title: v => `🆕 Imperivm Resurrected ${v}`,
    installed: ['**Hai già la mod installata?**', 'Entra nel gioco e ti comparirà l\'avviso per aggiornarla.'],
    fresh: site => ['**Non ce l\'hai ancora?**', `Scaricala dal sito: **[imperivmresurrected.com](${site})**`],
    news: '**📋 NOVITÀ**',
    firstOld: ['**Arrivi dalla 0.9.0 o precedente?**', 'Decomprimi lo zip a mano sulla cartella del gioco, una sola volta: l\'aggiornamento del gioco non può installare questa versione.'],
    firstNews: '**📋 PRIMA VERSIONE**',
    more: site => `Altro sul [sito](${site}).`,
  },
};

function releaseInfo() {
  if (notesFile) return { body: fs.readFileSync(notesFile, 'utf8'), isPrerelease: false, publishedAt: new Date().toISOString() };
  const out = execFileSync('gh', ['release', 'view', tag, '--json', 'body,isPrerelease,publishedAt'], { encoding: 'utf8' });
  return JSON.parse(out);
}

function sections(body) {
  const out = {};
  let current = null;
  for (const line of body.replace(/\r\n/g, '\n').split('\n')) {
    const heading = /^#{1,3}\s+(.+?)\s*$/.exec(line);
    if (heading) {
      const name = heading[1].replace(/[^\p{L}\s]/gu, '').trim();
      const lang = Object.keys(LANGUAGES).find(l => LANGUAGES[l].heading.test(name));
      if (lang) {
        current = lang;
        out[lang] = [];
        continue;
      }
    }
    if (current) out[current].push(line);
  }
  for (const lang of Object.keys(out)) out[lang] = out[lang].join('\n').trim();
  return out;
}

function clip(text, site, more) {
  if (text.length <= NOTES_LIMIT) return text;
  const cut = text.slice(0, NOTES_LIMIT);
  const end = cut.lastIndexOf('\n');
  return (end > NOTES_LIMIT / 2 ? cut.slice(0, end) : cut).trimEnd() + '\n…\n' + more(site);
}

function message(lang, version, notes, publishedAt, first) {
  const t = LANGUAGES[lang];
  const lines = first ? [...t.fresh(t.site), '', ...t.firstOld] : [...t.installed, '', ...t.fresh(t.site)];
  if (notes) lines.push('', first ? t.firstNews : t.news, clip(notes, t.site, t.more));
  const role = (process.env['DISCORD_ROLE_' + lang.toUpperCase()] || '').trim();
  return {
    content: role ? `<@&${role}>` : null,
    allowed_mentions: { parse: [], roles: role ? [role] : [] },
    embeds: [{ title: t.title(version), url: t.site, description: lines.join('\n'), color: RED, footer: { text: 'imperivmresurrected.com' }, timestamp: publishedAt }],
  };
}

async function main() {
  const hooks = Object.keys(LANGUAGES).map(lang => [lang, (process.env['DISCORD_WEBHOOK_' + lang.toUpperCase()] || '').trim()]);
  if (!dryRun && !hooks.some(([, url]) => url)) {
    console.log('no Discord webhook configured (DISCORD_WEBHOOK_ES/EN/IT): nothing to announce');
    return;
  }
  const release = releaseInfo();
  if (release.isPrerelease) {
    console.log(`${tag} is a pre-release: not announced`);
    return;
  }
  const version = tag.replace(/^v/i, '');
  const notes = sections(release.body || '');
  const first = /<!--\s*announce:\s*first-release\s*-->/.test(release.body || '');
  let failed = 0;
  for (const [lang, url] of hooks) {
    const payload = message(lang, version, notes[lang] || '', release.publishedAt || new Date().toISOString(), first);
    if (dryRun) {
      console.log(`--- ${lang}${url ? '' : ' (no webhook: would be skipped)'}\n` + JSON.stringify(payload, null, 2));
      continue;
    }
    if (!url) {
      console.log(`${lang}: no webhook, skipped`);
      continue;
    }
    const res = await fetch(url + (url.includes('?') ? '&' : '?') + 'wait=true', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    });
    if (res.ok) {
      console.log(`${lang}: announced`);
    }
    else {
      failed++;
      console.log(`${lang}: Discord answered ${res.status} ${(await res.text()).slice(0, 300)}`);
    }
  }
  if (failed) process.exitCode = 1;
}

main().catch(e => {
  console.error(e.message || e);
  process.exitCode = 1;
});
