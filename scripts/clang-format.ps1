foreach($file in Get-ChildItem -recurse -include *.cpp,*.h,*.js)
{
    clang-format -style=file -i $file
}