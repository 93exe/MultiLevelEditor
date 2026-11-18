
#include <Geode/Geode.hpp>

using namespace geode::prelude;

// Временная безопасная заглушка.
// Не создаёт дополнительные сцены и редакторы.
$on_mod(Loaded) {
    log::info("Multi-Level Editor loaded safely.");
}
