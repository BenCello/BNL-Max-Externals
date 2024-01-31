#! /bin/zsh -

#source ./secrets.sh

echo "$CerticateCommonName"

# remove existing dmg
rm -f -- ../mac_externals.notarized.dmg

# create a subfolder with mac external only
mkdir ../mac_externals
mv $(find ../externals -d 1 -iname '*.mxo') '../mac_externals'

# remove quarantine flag & codesign each external
sudo xattr -r -d com.apple.quarantine $(find '../mac_externals' -d 1 -iname '*.mxo')
#echo $MyPassword | sudo -S xattr -r -d com.apple.quarantine $(find '../mac_externals' -d 1 -iname '*.mxo')

if [[ -d ../mac_externals/bc.upnpc.mxo ]]; then
  codesign --verbose --deep --timestamp --force --sign "$CerticateCommonName" $(find '../mac_externals/bc.upnpc.mxo' -d 3 -type f -iname 'libminiupnpc.*')
fi
codesign --verbose --deep --timestamp --force --sign "$CerticateCommonName" $(find '../mac_externals' -d 1 -iname '*.mxo')

# create dmg with the externals
hdiutil create ../mac_externals.notarized.dmg -fs HFS+ -srcfolder ../mac_externals -ov
# notarize & staple dmg
#xcrun notarytool submit ../mac_externals.notarized.dmg --keychain-profile $AppSpecificPasswordName --wait
xcrun notarytool submit ../mac_externals.notarized.dmg --apple-id $MyAppleID --team-id $MyAppleTeam --password $AppSpecificPassword --wait
xcrun stapler staple ../mac_externals.notarized.dmg

# cleanup
mv $(find '../mac_externals' -d 1 -iname '*') '../externals'
rm -r '../mac_externals'