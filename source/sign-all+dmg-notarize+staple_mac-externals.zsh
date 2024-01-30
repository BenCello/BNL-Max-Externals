#! /bin/zsh -

#SigningID=E001E13C2F610B59C3176470422F35264A51FF09
#AppSpecificPasswordName=""

# remove existing dmg
rm -f -- ../mac_externals.notarized.dmg

# create a subfolder with mac external only
mkdir ../mac_externals
mv $(find ../externals -d 1 -iname '*.mxo') '../mac_externals'

# remove quarantine flag & codesign each external
echo $MyPassword | sudo -S xattr -r -d com.apple.quarantine $(find '../mac_externals' -d 1 -iname '*.mxo')
codesign --deep --timestamp --force --keychain $HOME/Library/Keychains/login.keychain -s $SigningID $(find '../mac_externals' -d 1 -iname '*.mxo')

# create dmg with the externals
#hdiutil create ../mac_externals.notarized.dmg -fs HFS+ -srcfolder ../mac_externals -ov
# notarize & staple dmg
#xcrun notarytool submit ../mac_externals.notarized.dmg --keychain-profile $AppSpecificPasswordName --wait
#xcrun stapler staple ../mac_externals.notarized.dmg

# cleanup
#mv $(find '../mac_externals' -d 1 -iname '*') '../externals'
#rm -r '../mac_externals'