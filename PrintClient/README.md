### EBMS Printing


The output for a print job is downloaded to the user's `Downloads` directory.  The name of the downloaded file is `PrintJob99999.tar` where `99999` represents the number of the print job.

These `*.tar` files used to be printed as a large package on a dedicated printer using the command:

```
C:> RunEbmsPrintJob 99999
```
However, this dedicated printer isn't reliable anymore and the users are now instead only printing portions of this print package. In order to print selected documents, those documents first need to be extracted from the `*.tar` file.

Open a command prompt and change the directory, if necessary, to 

```
C:\CDR\EBMS\Bin
```

Then run the script `ExtractPrintJob.cmd` to extract the files using the following command:

```
C:> ExtractPrintJob 99999
```
This command will extract the content of the file `PrintJob99999.tar` into the directory
 
```
Downloads\PrintJobs\PrintJob99999
```
from where the required `*.pdf` files can be printed on the printer of choice.
