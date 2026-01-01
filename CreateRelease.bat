@for %%Name IN (Win32 x64 ARM64) ( 
  7z a -bd -bso0 Bin\HomeMaticNPPLexer_%Name%.zip .\Bin\%Name%\Release\*.dll .\Bin\%Name%\Release\*.xml 
)
@Echo Done
