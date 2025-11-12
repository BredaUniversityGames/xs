# xs Documentation - Pure HTML Workflow

This documentation site uses **pure HTML/CSS** (no Jekyll, no build process needed).

## Quick Start

### 1. Local Development

Start a local server to preview changes:

```bash
cd /home/user/xs/docs
python3 -m http.server 8000
```

Then open http://localhost:8000 in your browser.

### 2. Edit HTML Files

Edit any HTML file directly:
- `index.html` - Landing page
- `getting-started.html` - Getting started guide
- `api/*.html` - API documentation (auto-generated)
- `css/style.css` - All styling

### 3. Generate API Documentation

When Wren module files change, regenerate API docs:

```bash
cd /home/user/xs
python3 tools/docs.py
```

This reads `assets/modules/*.wren` and generates `docs/api/*.html`.

### 4. Commit and Push

```bash
cd /home/user/xs
git add docs/
git commit -m "Update documentation"
git push
```

GitHub Pages will serve the HTML files directly (no build process).

## File Structure

```
docs/
├── .nojekyll          # Tells GitHub Pages to skip Jekyll
├── index.html         # Landing page
├── getting-started.html
├── api/
│   ├── index.html     # API reference overview
│   ├── render.html    # Auto-generated
│   ├── math.html      # Auto-generated
│   └── ...
├── css/
│   └── style.css      # Complete styling
├── js/
│   └── main.js        # Navigation & interactions
└── img/               # Images
```

## Claude Code CLI Workflow

### Ask Claude to Make Changes

```
"Update the landing page hero text"
"Add a new section to the getting started guide"
"Change the gradient colors to blue/purple"
"Fix the navigation menu styling"
```

### Regenerate API Docs After Wren Changes

```
"The Render API changed, regenerate the docs"
"I added new methods to xs_math.wren, update the API docs"
```

### Preview Changes

```
"Start the local server so I can preview"
"What's the URL to preview locally?"
```

### Commit Changes

```
"Commit these changes with message: [your message]"
"Push all documentation updates"
```

## No Build Process Required

- ✅ Edit HTML/CSS directly
- ✅ Refresh browser to see changes
- ✅ Python HTTP server for local preview
- ❌ No Jekyll installation needed
- ❌ No bundle install
- ❌ No Ruby dependencies
- ❌ No build step

## Technology Stack

- **Pure HTML5** for structure
- **CSS3** with gradient theme matching the engine
- **Vanilla JavaScript** for navigation
- **FontAwesome** for icons (CDN)
- **Python script** to generate API docs from Wren files

Simple, fast, and easy to maintain.
