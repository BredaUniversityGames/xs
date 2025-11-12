# Claude Code CLI Instructions for xs Documentation

This file contains instructions for Claude Code CLI to help maintain the xs documentation website.

## Website Overview

The xs documentation is a **pure HTML website** (no build process required):
- Location: `/home/user/xs/docs/`
- Technology: HTML5 + CSS3 + Vanilla JavaScript
- Theme: Orange/pink/purple gradient matching the engine
- Deployment: GitHub Pages (automatic, no Jekyll)

## File Structure

```
docs/
├── .nojekyll              # Tells GitHub Pages to skip Jekyll
├── index.html             # Landing page
├── getting-started.html   # Tutorial
├── api/
│   ├── index.html         # API overview
│   ├── render.html        # Auto-generated
│   ├── math.html          # Auto-generated
│   └── ...
├── css/style.css          # All styling
├── js/main.js             # Navigation logic
└── img/                   # Images
```

## Common Tasks

### 1. Edit HTML Pages

**User Request:** "Update the landing page to add..."

**Action:**
- Edit `docs/index.html` (or other HTML file)
- Test locally: `python3 -m http.server 8000` in docs/
- Commit and push

### 2. Regenerate API Documentation

**User Request:** "I updated the Wren modules, regenerate the docs"

**Action:**
```bash
cd /home/user/xs
python3 tools/docs_html.py
```

This reads `assets/modules/*.wren` and generates HTML files in `docs/api/`.

### 3. Update Styling

**User Request:** "Change the gradient to blue/purple"

**Action:**
- Edit `docs/css/style.css`
- Update CSS variables at the top
- Test locally

### 4. Preview Locally

**User Request:** "Start the local server"

**Action:**
```bash
cd /home/user/xs/docs
python3 -m http.server 8000
```

Then tell user to open http://localhost:8000

### 5. Commit Changes

Always commit and push after making changes:
```bash
git add docs/
git commit -m "Update documentation"
git push
```

## Important Notes

1. **No Build Process**: Just edit HTML/CSS directly, no compilation needed
2. **Auto-Deploy**: GitHub Pages serves the HTML files directly after push
3. **Testing**: Use Python's HTTP server for local preview
4. **API Docs**: Run `tools/docs_html.py` when Wren files change
5. **Icons**: Uses FontAwesome CDN (already included)

## Workflow Example

```
User: "Add a new section about multiplayer to the getting started page"

Claude:
1. Read docs/getting-started.html
2. Add new <h2> section with multiplayer content
3. Write the changes
4. Tell user to test: cd docs && python3 -m http.server 8000
5. Commit: git add docs/getting-started.html && git commit -m "Add multiplayer section"
6. Push: git push
```

## CSS Variables

Located at top of `docs/css/style.css`:
```css
:root {
  --xs-orange: #f89020;
  --xs-pink: #e85a8f;
  --xs-purple: #c739d8;
  --xs-dark: #1a1a2e;
  --xs-darker: #0f0f1e;
}
```

Change these to update the color scheme globally.

## Navigation Structure

Navigation is defined in each HTML file's `<nav>` section:
- Home → index.html
- Getting Started → getting-started.html
- API Reference → api/index.html
- Download → itch.io (external)
- GitHub → repository (external)

## When User Says...

| User Request | Action |
|--------------|--------|
| "Update the homepage" | Edit `docs/index.html` |
| "Change the colors" | Edit `docs/css/style.css` |
| "Regenerate API docs" | Run `python3 tools/docs_html.py` |
| "Start local server" | `cd docs && python3 -m http.server 8000` |
| "Add a new API page" | Create new HTML in `docs/api/` using template |
| "Fix navigation" | Edit `<nav>` section in affected HTML files |

## Git Branch

Always work on: `claude/update-website-011CV3zDQ1keD3cppENVPZvt` (or current session branch)

## Success Criteria

After changes:
- ✅ HTML validates (well-formed)
- ✅ Local preview works (port 8000)
- ✅ Committed and pushed to branch
- ✅ User can view changes locally before merge
