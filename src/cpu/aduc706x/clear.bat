@rem clear all the temp files in current folder
@for /R %%i in (Elves ExampleIAR ExampleRV Help LibRV Debug settings *.ewp *.dep *.ewd *.eww) do @(
        @echo del %%i
        @rm -rf "%%i"
)