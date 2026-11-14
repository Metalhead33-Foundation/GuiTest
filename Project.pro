TEMPLATE = subdirs

# Order matters: Elvavena is a utility lib, Euphemy likely uses it
SUBDIRS = Elvavena Euphemy Lotte Haruka Kaldi KaldiGL KaldiGLES KaldiVK KaldiD3D11 KaldiD3D12 UnitTests Sandbox

KaldiGL.depends = Kaldi
KaldiGLES.depends = Kaldi
KaldiVK.depends = Kaldi
KaldiD3D11.depends = Kaldi
KaldiD3D12.depends = Kaldi
Euphemy.depends = Elvavena
Lotte.depends = Elvavena Euphemy Kaldi
Haruka.depends = Elvavena Euphemy

# UnitTests is the automated (Catch2) suite; Sandbox is the manual playground.
UnitTests.depends = Euphemy Elvavena Lotte Haruka
Sandbox.depends = Euphemy Elvavena Lotte Haruka
