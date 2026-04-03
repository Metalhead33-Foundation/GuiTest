TEMPLATE = subdirs

# Order matters: Elvavena is a utility lib, Euphemy likely uses it
SUBDIRS = Elvavena Euphemy Lotte Haruka Test

Euphemy.depends = Elvavena
Lotte.depends = Elvavena
Haruka.depends = Elvavena
Test.depends = Euphemy Elvavena
