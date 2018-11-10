/**
 * @file ypm/ypm.c
 *
 * Yori shell update tool
 *
 * Copyright (c) 2018 Malcolm J. Smith
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>
#include <yoripkg.h>

/**
 Help text to display to the user.
 */
const
CHAR strYpmHelpText[] =
        "\n"
        "Installs or upgrades packages.\n"
        "\n"
        "YPM [-license]\n"
        "YPM -c <file> <pkgname> <version> <arch> -filelist <file> [-upgradepath <path>]\n"
        "       [-sourcepath <path>] [-symbolpath <path>] [-replaces <packages>]\n"
        "YPM -cs <file> <pkgname> <version> -filepath <directory>\n"
        "YPM -d <pkg>\n"
        "YPM -i <file>\n"
        "YPM -l\n"
        "YPM -ri [-a <arch>] [-v <version>] <pkgname>...\n"
        "YPM -rl\n"
        "YPM -src [<pkg>]\n"
        "YPM -sym [<pkg>]\n"
        "YPM [-a <arch>] -u [<pkg>]\n"
        "\n"
        "   -a             Specify a CPU architecture to upgrade to\n"
        "   -c             Create a binary package\n"
        "   -cs            Create a source package\n"
        "   -d             Delete an installed package\n"
        "   -i             Install a package from a specified file or URL\n"
        "   -l             List all currently installed packages\n"
        "   -ri            Install packages from remote servers\n"
        "   -rl            List available packages on remote servers\n"
        "   -src           Install source for specified package or all packages\n"
        "   -sym           Install debug symbols for specified package or all packages\n"
        "   -u             Upgrade a package or all currently installed packages\n";

/**
 Display usage text to the user.
 */
BOOL
YpmHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ypm %i.%i\n"), YPM_VER_MAJOR, YPM_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYpmHelpText);
    return TRUE;
}

/**
 A list of operations that the tool can perform.
 */
typedef enum _YPM_OPERATION {
    YpmOpNone = 0,
    YpmOpInstall = 1,
    YpmOpListPackages = 2,
    YpmOpUpgradeInstalled = 3,
    YpmOpDeleteInstalled = 4,
    YpmOpCreateBinaryPackage = 5,
    YpmOpCreateSourcePackage = 6,
    YpmOpRemoteList = 7,
    YpmOpInstallRemote = 8,
    YpmOpInstallSource = 9,
    YpmOpInstallSymbol = 10
} YPM_OPERATION;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the for builtin command.
 */
#define ENTRYPOINT YoriCmd_YPM
#else
/**
 The main entrypoint for the for standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the update cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;
    PYORI_STRING NewFileName = NULL;
    PYORI_STRING NewVersion = NULL;
    PYORI_STRING NewArch = NULL;
    PYORI_STRING NewName = NULL;
    PYORI_STRING SourcePath = NULL;
    PYORI_STRING UpgradePath = NULL;
    PYORI_STRING SymbolPath = NULL;
    PYORI_STRING FileList = NULL;
    PYORI_STRING FilePath = NULL;
    PYORI_STRING Replaces = NULL;
    DWORD ReplaceCount = 0;
    YPM_OPERATION Op;

    Op = YpmOpNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YpmHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                if (i + 1 < ArgC) {
                    NewArch = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (i + 4 < ArgC) {
                    NewFileName = &ArgV[i + 1];
                    NewName = &ArgV[i + 2];
                    NewVersion = &ArgV[i + 3];
                    NewArch = &ArgV[i + 4];
                    i += 4;
                    ArgumentUnderstood = TRUE;
                    Op = YpmOpCreateBinaryPackage;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("cs")) == 0) {
                if (i + 3 < ArgC) {
                    NewFileName = &ArgV[i + 1];
                    NewName = &ArgV[i + 2];
                    NewVersion = &ArgV[i + 3];
                    i += 3;
                    ArgumentUnderstood = TRUE;
                    Op = YpmOpCreateSourcePackage;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                Op = YpmOpDeleteInstalled;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("filelist")) == 0) {
                if (i + 1 < ArgC) {
                    FileList = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("filepath")) == 0) {
                if (i + 1 < ArgC) {
                    FilePath = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                Op = YpmOpInstall;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                Op = YpmOpListPackages;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("replaces")) == 0) {
                if (Op == YpmOpCreateBinaryPackage &&
                    i + 1 < ArgC) {

                    ArgumentUnderstood = TRUE;
                    Replaces = &ArgV[i + 1];

                    while (i + 1 < ArgC &&
                           !YoriLibIsCommandLineOption(&ArgV[i + 1], &Arg)) {

                        ReplaceCount++;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("ri")) == 0) {
                Op = YpmOpInstallRemote;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("rl")) == 0) {
                Op = YpmOpRemoteList;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("sourcepath")) == 0) {
                if (i + 1 < ArgC) {
                    SourcePath = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("src")) == 0) {
                Op = YpmOpInstallSource;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("sym")) == 0) {
                Op = YpmOpInstallSymbol;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("symbolpath")) == 0) {
                if (i + 1 < ArgC) {
                    SymbolPath = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("u")) == 0) {
                Op = YpmOpUpgradeInstalled;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("upgradepath")) == 0) {
                if (i + 1 < ArgC) {
                    UpgradePath = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("v")) == 0) {
                if (i + 1 < ArgC) {
                    NewVersion = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartArg = i + 1;
                break;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (Op == YpmOpNone) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: missing operation\n"));
        return EXIT_FAILURE;
    }

    if (Op == YpmOpInstall) {
        if (StartArg == 0 || StartArg >= ArgC) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: missing argument\n"));
            return EXIT_FAILURE;
        }

        for (i = StartArg; i < ArgC; i++) {
            YoriPkgInstallPackage(&ArgV[i], NULL);
        }

    } else if (Op == YpmOpListPackages) {
        YoriPkgListInstalledPackages();
    } else if (Op == YpmOpUpgradeInstalled) {
        if (StartArg == 0 || StartArg >= ArgC) {
            YoriPkgUpgradeInstalledPackages(NewArch);
        } else {
            for (i = StartArg; i < ArgC; i++) {
                YoriPkgUpgradeSinglePackage(&ArgV[i], NewArch);
            }
        }
    } else if (Op == YpmOpDeleteInstalled) {
        i = StartArg;
        if (StartArg == 0 || StartArg >= ArgC) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: missing argument\n"));
            return EXIT_FAILURE;
        }
        for (i = StartArg; i < ArgC; i++) {
            YoriPkgDeletePackage(NULL, &ArgV[i], TRUE);
        }
    } else if (Op == YpmOpCreateBinaryPackage) {
        ASSERT(NewFileName != NULL && NewName != NULL && NewVersion != NULL && NewArch != NULL);
        if (FileList == NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: missing file list\n"));
            return EXIT_FAILURE;
        }
        YoriPkgCreateBinaryPackage(NewFileName, NewName, NewVersion, NewArch, FileList, UpgradePath, SourcePath, SymbolPath, Replaces, ReplaceCount);
    } else if (Op == YpmOpCreateSourcePackage) {
        ASSERT(NewFileName != NULL && NewName != NULL && NewVersion != NULL);
        if (FilePath == NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: missing file tree root\n"));
            return EXIT_FAILURE;
        }
        YoriPkgCreateSourcePackage(NewFileName, NewName, NewVersion, FilePath);
    } else if (Op == YpmOpRemoteList) {
        YoriPkgDisplayAvailableRemotePackages();
    } else if (Op == YpmOpInstallRemote) {
        DWORD PkgCount;
        DWORD SuccessCount;
        PkgCount = ArgC - StartArg;
        if (PkgCount == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: missing package name\n"));
            return EXIT_FAILURE;
        }
        SuccessCount = YoriPkgInstallRemotePackages(&ArgV[StartArg], PkgCount, NULL, NewVersion, NewArch);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%i packages installed (%i not installed)\n"), SuccessCount, PkgCount - SuccessCount);
    } else if (Op == YpmOpInstallSource) {
        if (StartArg == 0 || StartArg >= ArgC) {
            YoriPkgInstallSourceForInstalledPackages();
        } else {
            for (i = StartArg; i < ArgC; i++) {
                YoriPkgInstallSourceForSinglePackage(&ArgV[i]);
            }
        }
    } else if (Op == YpmOpInstallSymbol) {
        if (StartArg == 0 || StartArg >= ArgC) {
            YoriPkgInstallSymbolsForInstalledPackages();
        } else {
            for (i = StartArg; i < ArgC; i++) {
                YoriPkgInstallSymbolForSinglePackage(&ArgV[i]);
            }
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
