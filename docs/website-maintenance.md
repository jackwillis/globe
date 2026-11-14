# Website maintenance

From the repository root, run `python3 -m http.server 8000 --bind 127.0.0.1 --directory site`
and open <http://127.0.0.1:8000/>. To exercise a project subpath, serve the
repository root with `python3 -m http.server 8000 --bind 127.0.0.1` and open
<http://127.0.0.1:8000/site/>. The stylesheet and future screenshot URLs
are relative to the page. The site has no build step or app-build dependency. Archivo and Leckerli One load from Google Fonts when available. The page falls back to local fonts offline.

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
OS 9 captures, store optimized images under `site/images/` and reuse them
in the README where appropriate. Add a figure at the screenshot comment in the hero
using a relative image URL, intrinsic width and height, descriptive alt text
and a caption naming the captured version and OS. Add one or two supporting
figures only if they show distinct features. Keep capture provenance with #118.
