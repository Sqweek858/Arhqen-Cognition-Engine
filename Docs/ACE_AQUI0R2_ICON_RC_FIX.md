# ACE-AQUI0R2 Icon / RC Fix

## Problem

`ArhqenCognitionEngine.rc` referenced:

```text
Resources\ArhqenCognitionEngine.ico
```

but the package only contained:

```text
Resources\ArhqenAI.ico
```

This caused Resource Compiler error:

```text
RC2135: file not found: Resources\ArhqenCognitionEngine.ico
```

## Fix

The existing icon asset was preserved and copied to the official rebranded filename:

```text
Source\Resources\ArhqenCognitionEngine.ico
```

The Visual Studio project now also includes the rebranded icon path.

## Runtime changes

None.

This patch only fixes the missing resource file / project reference.
