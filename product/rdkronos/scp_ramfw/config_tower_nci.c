
#include <mod_tower_nci.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>

#include <fmw_cmsis.h>

struct fwk_module_config config_tower_nci = {
    .elements = FWK_MODULE_STATIC_ELEMENTS({ { 0 } })
};
