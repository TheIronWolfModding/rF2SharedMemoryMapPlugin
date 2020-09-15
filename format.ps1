pushd .
Get-ChildItem -Path . -Directory -Recurse |
    foreach {
        cd $_.FullName
        if (-not $_.FullName.Contains("InternalsPlugin.hpp") -and -not $_.FullName.Contains("RFPluginObjects.hpp")) { # exclude ISI headers
            Get-ChildItem -Path . -File -Recurse | foreach {
                #echo $_.FullName 
                if ($_.Extension -eq ".h" -or $_.Extension -eq ".hpp" -or $_.Extension -eq ".cxx" -or $_.Extension -eq ".cpp") {
                    if (-not $_.Name.Contains(".gen")) {  # exclude generated headers
                         echo $_.FullName 
                         & "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\vcpackages\clang-format.exe" -i -style=file $_.FullName
                    }
                }
            }
        } else {
            Write-Host -ForegroundColor Yellow "Skip folder: " $_.FullName
        }
        
        #&clang-format -i -style=WebKit *.cpp
    }
    
 popd