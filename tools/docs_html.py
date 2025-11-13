"""
# WrenDoc HTML Generator
Generates HTML documentation from Wren files with documentation comments.

Supports two comment styles:
- /** ... */ (block comments)
- /// (C#-style line comments)

Special commands:
- doc-disable: Skip this file entirely
- doc-name: CustomFileName: Set the output filename
- doc-header: Add a custom header/title

Generates pure HTML files in docs/api/ directory.
"""

class config:
    repo = "github.com/BredaUniversityGames/xs"
    branch = "blob/main"
    docs = "docs/api"
    path = "assets/modules"

import re
import sys
from pathlib import Path
import html as html_module

CMD_DISABLE = "doc-disable"
CMD_FILENAME = "doc-name:"
CMD_HEADER = "doc-header"

def commandIsDisable(comment):
    return comment.strip().startswith(CMD_DISABLE)

def commandIsFilename(comment):
    return comment.strip().startswith(CMD_FILENAME)

def commandIsHeader(comment):
    return comment.strip().startswith(CMD_HEADER)

def getPath(path=None):
    return path or "./"

def getDocPath(path=None):
    return f"{getPath(path)}{config.docs}"

def getFiles(path=None):
    path = getPath(path)
    files = list(Path(path).glob('**/*.wren'))
    return files

def getFileContent(path):
    with open(path) as file:
        return file.read()

def getFileInfo(path):
    return Path(path)

def getLineNumber(index, content):
    line = 1
    for pos, char in enumerate(content):
        if char == "\n":
            line += 1
        if index == pos:
            return line
    return None

def getTripleSlashComments(content, file):
    """Extract /// style documentation comments"""
    tripleSlashRegex = re.compile(r"^\s*///(.*)$", re.MULTILINE)
    comments = []
    lines = content.split('\n')
    i = 0
    filename = None
    header = ""

    while i < len(lines):
        line = lines[i]
        match = tripleSlashRegex.match(line)

        if match:
            commentLines = []
            startLineIdx = i

            while i < len(lines) and tripleSlashRegex.match(lines[i]):
                commentMatch = tripleSlashRegex.match(lines[i])
                commentText = commentMatch.group(1)
                if commentText.startswith(' '):
                    commentText = commentText[1:]
                commentLines.append(commentText)
                i += 1

            comment = '\n'.join(commentLines)

            if commandIsDisable(comment):
                return []
            if commandIsFilename(comment):
                filename = comment[comment.find(CMD_FILENAME) + len(CMD_FILENAME):].strip()
                continue
            if commandIsHeader(comment):
                header = comment[comment.find(CMD_HEADER) + len(CMD_HEADER):]
                continue

            nextline = ""
            nextlineStart = i
            while i < len(lines) and lines[i].strip() == "":
                i += 1

            if i < len(lines):
                # Get the line up to (but not including) the opening brace
                full_nextline = lines[i]
                brace_pos = full_nextline.find('{')
                if brace_pos >= 0:
                    nextline = full_nextline[:brace_pos].rstrip()  # Exclude the brace and trim whitespace
                else:
                    nextline = full_nextline

            start = sum(len(lines[j]) + 1 for j in range(startLineIdx))
            end = start + sum(len(lines[j]) + 1 for j in range(startLineIdx, nextlineStart))
            nextlineStartPos = sum(len(lines[j]) + 1 for j in range(i))
            nextlineEndPos = nextlineStartPos + len(nextline)

            lineno = startLineIdx + 1
            nextlineno = i + 1

            comments.append(
                (
                    (comment, start, end, lineno),
                    (nextline, nextlineStartPos, nextlineEndPos, nextlineno),
                    (0, filename, header)
                )
            )

        i += 1

    return comments

def getComments(content, file):
    """Extract all documentation comments from file"""
    startCommentRegex = re.compile(r"\s*\S*(\/\*\*)")
    comments = []
    startMatches = startCommentRegex.finditer(content)
    endCommentChar = "*/"
    openCurlyBraceChar = "{"
    filename = None
    header = ""

    for startMatch in startMatches:
        start = startMatch.span()[1]
        end = content.find(endCommentChar, start)
        lineno = getLineNumber(start, content)

        if end >= 0:
            comment = content[start:end]
            spaces = (len(comment) - 1) - len(comment.lstrip())

            if comment != "":
                if commandIsDisable(comment):
                    return []
                if commandIsFilename(comment):
                    filename = comment[comment.find(CMD_FILENAME) + len(CMD_FILENAME):].strip()
                    continue
                if commandIsHeader(comment):
                    header = comment[comment.find(CMD_HEADER) + len(CMD_HEADER):]
                    continue

                nextlineStart = end + len(endCommentChar)
                nextlineEnd = content.find(openCurlyBraceChar, nextlineStart)
                if nextlineEnd >= 0:
                    nextline = content[nextlineStart:nextlineEnd].rstrip()  # Exclude brace and trim
                else:
                    nextline = content[nextlineStart:nextlineEnd]
                nextlineno = getLineNumber(nextlineStart, content)

                comments.append(
                    (
                        (comment, start, end, lineno),
                        (nextline, nextlineStart, nextlineEnd, nextlineno),
                        (spaces, filename, header)
                    )
                )

    # Also collect /// style comments
    tripleSlashComments = getTripleSlashComments(content, file)
    comments.extend(tripleSlashComments)

    # Sort by line number
    comments.sort(key=lambda x: x[0][3])

    return comments

def escape_html(text):
    """Escape HTML special characters"""
    return html_module.escape(text)

def markdown_to_html(text):
    """Convert simple markdown to HTML"""
    # Code blocks
    text = re.sub(r'`([^`]+)`', r'<code>\1</code>', text)
    # Bold
    text = re.sub(r'\*\*([^*]+)\*\*', r'<strong>\1</strong>', text)
    # Links
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', r'<a href="\2">\1</a>', text)
    return text

def saveFile(path, content):
    """Save content to file, creating directories if needed"""
    parent = Path(path).parent
    parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w+") as file:
        file.write(content)

def makeHTMLFile(comments, file):
    """Generate HTML documentation from comments"""
    info = getFileInfo(file)
    url = f"https://{config.repo}/{config.branch}/{info}"

    # Create friendly title from filename
    filename_base = info.stem
    title = filename_base.replace('_', ' ').title()

    # Map module names to friendly titles
    title_map = {
        "xs": "Core API (Render, Input, etc.)",
        "xs_math": "Math & Vectors",
        "xs_ec": "Entity-Component System",
        "xs_components": "Built-in Components",
        "xs_shapes": "Shapes",
        "xs_containers": "Containers",
        "xs_tools": "Tools",
    }

    if filename_base in title_map:
        title = title_map[filename_base]
    elif "assert" in filename_base:
        title = "Assert"

    # Start building HTML
    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>{title} - xs game engine API</title>
  <meta name="description" content="API documentation for {title}">

  <!-- FontAwesome Icons -->
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">

  <!-- Custom CSS -->
  <link rel="stylesheet" href="../css/style.css">

  <!-- Favicon -->
  <link rel="icon" type="image/png" href="../img/icon_tiny.webp">
</head>
<body>
  <!-- Navigation -->
  <nav>
    <a href="../index.html" class="nav-logo">
      <img src="../img/xs_logo_animated.svg" alt="xs logo">
    </a>
    <ul class="nav-links">
      <li><a href="../index.html" class="logo"><img src="../img/xs_icon.svg" alt="xs" class="icon-svg">game engine</a></li>
      <li><a href="../getting-started.html"><i class="fas fa-rocket"></i> getting started</a></li>
      <li><a href="index.html"><i class="fas fa-book"></i> api reference</a></li>
      <li><a href="https://xs-engine.itch.io/xs" target="_blank"><i class="fas fa-download"></i> download</a></li>
      <li><a href="https://github.com/BredaUniversityGames/xs" target="_blank"><i class="fab fa-github"></i> github</a></li>
    </ul>
  </nav>

  <!-- Main Content -->
  <div class="container api-section">
    <h1>{title}</h1>
    <p><em>Source: <a href="{url}">{info}</a></em></p>
"""

    for index, comment in enumerate(comments):
        content_tuple, line_tuple, meta = comment
        spaces, filename, header = meta

        content, cstart, cend, clineno = content_tuple
        line, lstart, lend, llineno = line_tuple

        line = line.strip()

        # Check if it's a class declaration
        isClass = False
        className = ""

        if line.startswith("foreign class "):
            isClass = True
            className = line[len("foreign class "):].split()[0]
        elif line.startswith("class "):
            isClass = True
            className = line[len("class "):].split()[0]

        if isClass:
            html += f'\n<hr>\n<h2>Class {escape_html(className)} <a href="{url}#L{llineno}" style="font-size: 0.6em; opacity: 0.6;">↗</a></h2>\n'
        else:
            # Method/property
            cleanLine = line.replace("foreign ", "")
            html += f'\n<h3><code>{escape_html(cleanLine)}</code> <a href="{url}#L{llineno}" style="font-size: 0.7em; opacity: 0.6;">↗</a></h3>\n'

        # Add comment content
        content_lines = content.split('\n')
        html += '<div class="api-method">\n'
        for content_line in content_lines:
            stripped = content_line[spaces:].rstrip() if len(content_line) > spaces else content_line.strip()
            if stripped:
                # Convert markdown to HTML
                html_line = markdown_to_html(escape_html(stripped))
                html += f'<p>{html_line}</p>\n'
        html += '</div>\n'

    # Close HTML
    html += """
  </div>

  <!-- Footer -->
  <footer>
    <p>xs - lovingly made at Breda University of Applied Sciences</p>
    <p><a href="https://github.com/BredaUniversityGames/xs">GitHub</a> | <a href="https://xs-engine.itch.io/xs">Download</a></p>
  </footer>

  <!-- JavaScript -->
  <script src="../js/main.js"></script>
</body>
</html>
"""

    # Determine output filename - use friendly names
    filename_map = {
        "xs": "render",
        "xs_math": "math",
        "xs_ec": "ec",
        "xs_components": "components",
        "xs_shapes": "shapes",
        "xs_containers": "containers",
        "xs_tools": "tools",
    }

    name = filename_base
    if filename_base in filename_map:
        name = filename_map[filename_base]
    elif "assert" in filename_base:
        name = "assert"

    if filename:  # Override if custom name specified
        name = filename

    doc = f"{getDocPath()}/{name}.html"
    saveFile(doc, html)
    print(f"Generated: {doc}")

def main():
    files = getFiles(config.path)
    total = 0
    for file in files:
        content = getFileContent(file)
        comments = getComments(content, file)
        if len(comments) > 0:
            total += len(comments)
            makeHTMLFile(comments, file)

    print(f"\\nJobs Done! Found {len(files)} files, parsed {total} comments.")
    print(f"HTML docs saved in `{getDocPath()}`")

if __name__ == "__main__":
    main()
