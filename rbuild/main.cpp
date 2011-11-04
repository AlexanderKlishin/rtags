#include "RBuild.h"
#include "Path.h"
#include "Precompile.h"
#include <QCoreApplication>
#include <QDir>
#include <getopt.h>
#include <RTags.h>
#include <AtomicString.h>

static inline void usage(const char* argv0, FILE *f)
{
    fprintf(f,
            "%s [options]...\n"
            "  --help|-h                  Display this help\n"
            "  --db-file|-d [arg]         Use this database file\n"
            "  --update|-u                Update database\n",
            argv0);
}

using namespace RTags;

class PrecompileScope
{
public:
    PrecompileScope() {}
    ~PrecompileScope() { Precompile::cleanup(); }
};

int main(int argc, char** argv)
{
    QCoreApplication::setOrganizationName("RTags");
    QCoreApplication::setOrganizationDomain("https://github.com/Andersbakken/rtags");
    QCoreApplication::setApplicationName("RTags");
    QCoreApplication app(argc, argv);
    Path db;
    QSet<Path> srcDirs;
    bool update = false;

    PrecompileScope prescope;

    struct option longOptions[] = {
        { "help", 0, 0, 'h' },
        { "db-file", 1, 0, 'd' },
        { "update", 0, 0, 'u' },
        { "source-dir", 1, 0, 's' },
        { 0, 0, 0, 0 },
    };
    const char *shortOptions = "hud:s:";

    int idx, longIndex;
    while ((idx = getopt_long(argc, argv, shortOptions, longOptions, &longIndex)) != -1) {
        switch (idx) {
        case '?':
            usage(argv[0], stderr);
            return 1;
        case 's':
            srcDirs.insert(Path::resolved(optarg));
            break;
        case 'h':
            usage(argv[0], stdout);
            return 0;
        case 'u':
            update = true;
            break;
        case 'd':
            db = QByteArray(optarg);
            break;
        default:
            printf("%s\n", optarg);
            break;
        }
    }

    if (update && !srcDirs.isEmpty()) {
        fprintf(stderr, "Can't use --source-dir with --update");
        return 1;

    }
    if (db.isEmpty()) {
        if (update) {
            db = findRtagsDb();
        } else {
            db = ".rtags.db";
        }
    }
    if (update && !db.exists()) {
        fprintf(stderr, "No db dir, exiting\n");
        return 1;
    }

    RBuild build;
    build.setDBPath(db);
    if (update) {
        build.updateDB();
        return 0;
    } else {
        bool ok;
        Path appPath = Path::resolved(QDir::currentPath().toLocal8Bit(), Path(), &ok);
        if (!ok)
            qFatal("Unable to resolve initial path");
        const char *makefile = "Makefile";
        if (optind < argc)
            makefile = argv[optind];
        Path p = Path::resolved(makefile, appPath);
        if (p.isDir())
            p += "/Makefile";
        if (!build.buildDB(p, srcDirs))
            return 1;
        return app.exec();
    }
}
