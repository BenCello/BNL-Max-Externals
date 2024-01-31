#! /bin/zsh -

#source ./secrets.sh

# Remove quarantine flag & codesign each external
# Manual version
#sudo xattr -r -d com.apple.quarantine $(find '../../externals' -d 1 -iname '*.mxo')
# Drone version
echo $MyPassword | sudo -S xattr -r -d com.apple.quarantine $(find '../../externals' -d 1 -iname '*.mxo')

if [[ -d ../../externals/bc.upnpc.mxo ]]; then
  codesign --verbose --deep --timestamp --force --sign "$CerticateCommonName" $(find '../../externals/bc.upnpc.mxo' -d 3 -type f -iname 'libminiupnpc.*')
fi
codesign --verbose --deep --timestamp --force --sign "$CerticateCommonName" $(find '../../externals' -d 1 -iname '*.mxo')
