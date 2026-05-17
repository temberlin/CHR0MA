# CHR0MA
a plug-in for after effects, replacement for hue/saturation.

This is heavily based off the work of Björn Ottosson (https://bottosson.github.io/posts/oklab/)

## Build Instructions

Clone this repo into the Examples/ directory of the After Effects SDK, then build it using your platforms respcetive buildsystem

## Making a new update

1. Bump version in CHR0MA.h
2. Bump AE_Effect_Version in CHR0MAPiPL.r, see definition of PF_VERSION macro for how to calculate
3. Bump MARKETING_VERSION in project.pbxproj (Identity -> Version in XCode)

### macOS Package instructions

1. Product -> Archive
2. Open in finder
3. Show package contents
4. Find the .plugin file
5. Send to lloyd
