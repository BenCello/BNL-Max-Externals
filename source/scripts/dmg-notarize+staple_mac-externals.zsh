#! /bin/zsh -

#source ./secrets.sh

# Remove existing dmg
rm -f -- '../../mac_externals.notarized.dmg'

# create a subfolder with mac external only
mkdir -p '../../mac_externals'
mv $(find '../../externals' -d 1 -iname '*.mxo') '../../mac_externals'

# create dmg with the externals
hdiutil create '../../mac_externals.notarized.dmg' -fs HFS+ -srcfolder '../../mac_externals' -ov
# notarize & staple dmg
xcrun notarytool submit '../../mac_externals.notarized.dmg' --apple-id $MyAppleID --team-id $MyAppleTeam --password $AppSpecificPassword --wait
xcrun stapler staple '../../mac_externals.notarized.dmg'

# cleanup
mv $(find '../../mac_externals' -d 1 -iname '*') '../../externals'
rm -r '../../mac_externals'
