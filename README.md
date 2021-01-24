# dwsanitizer
DirectWave (FL Studio's sampler) monolithic dwp sanitizer

## ATTENTION
THERE IS NO GUARANTEE OR WARRANTY AT ALL, USE AT YOUR OWN RISK.

dwp file is Image-line's proprietary format, no information is disclosed about it.
So, this tool might not be work as I intended. (I tested using DirectWave in FL STUDIO v20.6.2 build 1549, v20.7.2 build 1863)
 
## What is this
In the DirectWave monolithic *.dwp file, some user personal information will be included in the path.
For example, the path of dwp file itself, such a information seems to be unnecessary, is included in the binary like below:
```
   C:\Users\%USERNAME%\Documents\Image-Line\DirectWave\foo.dwp
```
**Note**: %USERNAME% is not recorded as environmental variable, actual raw value (might be sensitive information such as your name or enterprise ID or so) is recorded in the dwp file.

Or, monolithic dwp file contains waveform itself, so it doesn't require the path of the source sample wave file. But the information is recorded in the file like below:
```
   %SystemDrive%\PATH_TO\bar.wav
```
 
If you share a dwp files to the others, those information will be included unintentionally. I don't feel comfortable if those information were shared with the others.
This tool removes those information from monolithic dwp file.
 
## How to build
```sh
gcc dwsanitizer.c -o dwsanitizer
```
 
## How to use:
If you want to sanitize foo.dwp,
```sh
./dwsanitizer foo.dwp
```
 would generates O_foo.dwp and the file will be sanitized.
 
**Note**: Output filename is simply added prefix "O_" to input file, if the same filename is exists, it will be overwritten without notice. Do NOT specify the path to target dwp file, it's not supported.
 
## Limitation (Known bug):
The dwp file format is proprietary and not disclosed to public. So, this implementation is only an ad hoc based on my very limited research.

If waveforms in the monolithic dwp file contains binary sequences 0xF6 0x01 0x00 0x00 [ANY] 0x00 0x00 0x00 0x00 0x00 0x00 0x00, this tool would be in malfunction.

I recommend to keep original dwp file to prevent data loss or future incompatibilities.
