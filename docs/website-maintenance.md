# Website maintenance

From the repository root, run `python3 -m http.server 8000 --bind 127.0.0.1 --directory site`
and open <http://127.0.0.1:8000/>. To exercise a project subpath, serve the
repository root with `python3 -m http.server 8000 --bind 127.0.0.1` and open
<http://127.0.0.1:8000/site/>. The stylesheet and screenshot URLs are relative to the page. The site has no
build step or app-build dependency.

The current `site/images/` previews reuse the exact existing repository image
blobs from `docs/images/`: `os9-tiger-classic.png` is an OS 9 application
running in **Tiger Classic on a G4** (2026-11-02); `map.png` and
`relief.png` are **headless 640×480 renders**, not OS 9 GUI screenshots.
Captions and alt text distinguish these sources. Do not label these images as
bare Mac OS 9.2.2 hardware captures. Archivo and Leckerli One load from Google Fonts when available. The page falls back to local fonts offline.

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

When [#118](https://github.com/jackwillis/globe/issues/118) supplies authentic
bare OS 9 captures, keep the originals and conversion history, place optimized
copies in `site/images/`, and replace the **interim Tiger Classic hero** with
an accurately labeled bare OS 9 screenshot. Give it an intrinsic width and height,
descriptive alt text, and a caption naming the captured version and OS.
Use additional supporting figures only if they show distinct features, and keep
capture provenance with #118.
