"""
# WrenDoc
This is a simple documentation generator.
The idea is to parse wren files and look for
markdown style comments.

Supports two comment styles:
- /** ... */ (block comments)
- /// (C#-style line comments)

Special commands (work with both styles):
- doc-disable: Skip this file entirely
- doc-name: CustomFileName: Set the output filename (supports paths like "api/Math")
- doc-header: Add a custom header/title to the documentation

Examples:
  /// doc-name: api/MathModule

  /// doc-header
  /// # Math Module
  /// This module provides math utilities.

  /// Adds two numbers
  static add(a, b) { a + b }

Then generate the proper .md files inside docs
directory.
"""

class config:
    repo = "github.com/BredaUniversityGames/xs"
    branch = "blob/main"
    docs = "docs"
    path = "games/shared/modules"

import re
import glob
import sys
from pathlib import Path

# Avoid parsing the file
# e.g. /** doc-disable */
CMD_DISABLE = "doc-disable"

# Sets the filename for the doc
# e.g /** doc-name: MyDoc */
CMD_FILENAME = "doc-name:"

CMD_HEADER = "doc-header"

def commandIsDisable(comment):
  return comment.strip().startswith(CMD_DISABLE)

def commandIsFilename(comment):
  return comment.strip().startswith(CMD_FILENAME)

def commandIsHeader(comment):
  return comment.strip().startswith(CMD_HEADER)

def getPath(path = None):
  # Safely get the first arg
  # Use default path if not found
  path = path  or "./"
  return path

def getDocPath(path = None):
  return f"{getPath(path)}{config.docs}"

def getFiles(path = None):
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
  """Extract /// style documentation comments (C# style)"""
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
      # Found a /// comment, collect consecutive /// lines
      commentLines = []
      startLineIdx = i
      # For /// comments, we already strip the prefix, so no additional spaces to remove
      spaces = 0

      while i < len(lines) and tripleSlashRegex.match(lines[i]):
        commentMatch = tripleSlashRegex.match(lines[i])
        # Strip leading space from captured content if present
        commentText = commentMatch.group(1)
        if commentText.startswith(' '):
          commentText = commentText[1:]
        commentLines.append(commentText)
        i += 1

      # Join all comment lines
      comment = '\n'.join(commentLines)

      # Check for special commands
      if commandIsDisable(comment):
        return []

      if commandIsFilename(comment):
        filename = comment[comment.find(CMD_FILENAME) + len(CMD_FILENAME):].strip()
        continue

      if commandIsHeader(comment):
        header = comment[comment.find(CMD_HEADER) + len(CMD_HEADER):]
        continue

      # Find the next non-empty, non-comment line (the code being documented)
      nextline = ""
      nextlineStart = i
      while i < len(lines) and lines[i].strip() == "":
        i += 1

      if i < len(lines):
        nextline = lines[i]

      # Calculate character positions
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
          (spaces, filename, header)
        )
      )

    i += 1

  return comments

def getComments(content, file):
  # Regex for /** comment start
  startCommentRegex = re.compile(r"\s*\S*(\/\*\*)")

  # Regex for /// single-line comments (C# style)
  tripleSlashRegex = re.compile(r"^\s*///(.*)$", re.MULTILINE)

  # for all the found matches
  # search for the corresponding */
  # and save the comment inside
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
      # count leading spaces https://stackoverflow.com/a/13649013
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

  # Sort comments by line number to maintain document order
  comments.sort(key=lambda x: x[0][3])

  return comments

def getHref(line, lineno, url):
  return f"[{line}]({url}#L{lineno})\n"

def saveFile(path, content):
  # Create parent directories if they don't exist
  parent = Path(path).parent
  parent.mkdir(parents=True, exist_ok=True)

  file = open(path, "w+")
  file.write(content)
  file.close()

def makeMarkdownFile(comments, file):

  info = getFileInfo(file)
  # print(info, info.name, info.stem, info.parent)
  url = f"https://{config.repo}/{config.branch}/{info}"

  markdown = f"""<!-- file: {info} -->
<!-- documentation automatically generated using domepunk/tools/doc -->"""

  apiHeaderPresent = False
  addedHeaderContent = False

  for index, comment in enumerate(comments):
    content, line, meta = comment
    spaces, filename, header = meta

    if not addedHeaderContent:
      markdown += header
      addedHeaderContent = True

    content, cstart, cend, clineno = content
    line, lstart, lend, llineno = line

    line = line.strip()

    # Headers
    classKeyword = "class"

    # Check if line is a class declaration (handles "class" and "foreign class")
    isClass = False
    className = ""

    if line.startswith("foreign class "):
      isClass = True
      className = line[len("foreign class "):].split()[0]  # Get just the class name
    elif line.startswith("class "):
      isClass = True
      className = line[len("class "):].split()[0]  # Get just the class name

    if isClass:
      title = f"Class {className}"
      markdown += f"\n---\n## {getHref(title, llineno, url)}\n"
      apiHeaderPresent = False
    else:
      if not apiHeaderPresent:
        markdown += "\n## API\n"
        apiHeaderPresent = True
      # Remove "foreign" keyword from method signatures (implementation detail)
      cleanLine = line.replace("foreign ", "")
      markdown += f"\n### {getHref(cleanLine, llineno, url)}\n"

    # Write comment contents to the markdown file
    buffer = ""
    for char in content:
      buffer += char
      if char == "\n":
        # Strip leading spaces to have a proper markdown doc
        markdown += buffer[spaces:].rstrip() + char
        buffer = ""

    # Handle any remaining content that doesn't end with newline
    if buffer:
      markdown += buffer[spaces:].rstrip() + "\n"

  # End parsing and save file
  name = f"{info}".lower().strip().replace("/", "-").replace("\\", "-")
  if filename:
    name = filename

  doc = f"{getDocPath()}/{name}.md"
  saveFile(doc, markdown)


def main():
  files = getFiles(config.path)
  total = 0
  pending = 0
  for file in files:
    content = getFileContent(file)
    comments = getComments(content, file)
    pending = len(comments)
    if pending > 0:
      total += pending
      makeMarkdownFile(comments, file)

  print(f"Jobs Done!: found {len(files)} files. parsed {total} comments. docs saved in `{getDocPath()}`")

if __name__ == "__main__":
  main()