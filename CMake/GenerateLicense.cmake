function(AddLicenseFile _text _name _file)
  file(READ "${_file}" _fileText)
  set("${_text}" "${${_text}}\n\n#define ${_name}_LICENSE R\"___(${_fileText})___\"" PARENT_SCOPE)
endfunction()

function(AddLicenseStub _text _name _file _startLine _endLine)
endfunction()
