foreach($file in Get-ChildItem -recurse -include *.cpp,*.h,*.js,*.java)
{
    clang-format -style=file -i $file
}