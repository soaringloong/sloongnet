#include <string>
using namespace std;
#include "sqlite_orm/sqlite_orm.h"
using namespace sqlite_orm;

namespace Sloong
{
    const static string TEMPLATE_TBL_NAME = "template_list";

    struct TemplateInfo
    {
        int id;
        int replicas;
        int type;
        string configuation;
        string name;
        string note;
    };

    inline auto InitStorage(const string& path) {
        using namespace sqlite_orm;
        return make_storage(path,
            make_table(TEMPLATE_TBL_NAME,
                make_column("id", &TemplateInfo::id, autoincrement(), primary_key()),
                make_column("replicas", &TemplateInfo::replicas),
                make_column("type", &TemplateInfo::type),
                make_column("configuation", &TemplateInfo::configuation),
                make_column("name", &TemplateInfo::name),
                make_column("note", &TemplateInfo::note))
        );
    }

    using Storage = decltype(InitStorage(""));
}