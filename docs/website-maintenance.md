# Website maintenance

From the repository root, run `python3 -m http.server 8000 --bind 127.0.0.1 --directory site`
and open <http://127.0.0.1:8000/>. To exercise a project subpath, serve the
repository root with `python3 -m http.server 8000 --bind 127.0.0.1` and open
<http://127.0.0.1:8000/site/>. The stylesheet and screenshot URLs are relative to the page. The site has no
build step or app-build dependency.

The images in `site/images/` are cropped copies of **bare Mac OS 9.2.2 captures** from the
Power Mac G4 (2026-11-17, Command-Shift-3, 1280×960), converted from PICT to PNG without
scaling. They are PNG, not JPEG: the app draws flat 16-bit colour, and lossless PNG is smaller than
JPEG at quality 85-90 and keeps the text crisp. The full-screen masters, crop boxes and
provenance are in [images/os9/README.md](images/os9/README.md). The page keeps captions short (the hero
says "On a Power Mac G4."); the OS and build details live in that README. Do not mix in Tiger Classic
captures or headless renders without saying so.

Archivo and Leckerli One load from Google Fonts when available. The page falls
back to local fonts offline.

After owner approval, select **GitHub Actions** in Settings → Pages. Run
**Publish website** from `main` in the Actions tab for the initial publication
and after website updates. The workflow uploads only `site/`.

Confirm the account site's custom-domain configuration, the repository
name and the actual URL in Pages settings. As part of
[#124](https://github.com/jackwillis/globe/issues/124), sweep the GitHub links
in `site/index.html` and test the final asset URLs. Add the public website link
to the root README once the live URL works.
See [GitHub Pages publishing sources](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site).

Use the [release checklist](release-checklist.md) and its proposed website
steps in [PR #154](https://github.com/jackwillis/globe/pull/154). For each
new release, update the StuffIt button, version and release notes link,
corresponding source archive, tagged ReadMe/license/credits links, and tested
compatibility and shipped features together. Use the asset URLs from the
actual release. Check the live download and source archive as a visitor.

To change a screenshot, keep the master in `docs/images/os9/`, record the crop in its README, and
give each figure an intrinsic width and height, descriptive alt text and a caption naming the OS.
Use additional figures only if they show distinct features.
