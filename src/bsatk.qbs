import qbs.base 1.0
import qbs.File
import "../commonfunctions.js" as Common

StaticLibrary {
    name: 'BSAToolkit'

    Depends { name: 'cpp' }

    cpp.defines: []


    cpp.libraryPaths: Common.zlibLibraryPaths(qbs)
    cpp.staticLibraries: Common.zlibLibs(qbs)

    cpp.includePaths: [ qbs.getenv("BOOSTPATH"), qbs.getenv("ZLIBPATH"), qbs.getenv("ZLIBPATH") + "/build" ]

    files: [
        '*.cpp',
        '*.h',
        '*.ui'
    ]
}
