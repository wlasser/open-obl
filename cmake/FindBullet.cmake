find_package(PkgConfig)
pkg_check_modules(PC_Bullet QUIET Bullet)

find_path(Bullet_INCLUDE_DIR
        NAMES btBulletCollisionCommon.h
        PATHS ${PC_Bullet_INCLUDE_DIRS}
        PATH_SUFFIXES bullet)

find_path(Bullet_LIBRARY
        NAMES libBulletCollision.a
        PATHS ${PC_Bullet_LIBRARY_DIRS}
        PATH_SUFFIXES lib)

set(Bullet_VERSION ${PC_Bullet_VERSION})

mark_as_advanced(Bullet_FOUND Bullet_INCLUDE_DIR Bullet_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bullet
        FOUND_VAR Bullet_FOUND
        REQUIRED_VARS Bullet_INCLUDE_DIR Bullet_LIBRARY
        VERSION_VAR Bullet_VERSION)

if (Bullet_FOUND)
    set(Bullet_INCLUDE_DIRS ${Bullet_INCLUDE_DIR})
    set(Bullet_LIBRARIES ${Bullet_LIBRARY})
endif ()

if (Bullet_FOUND AND NOT TARGET Bullet::LinearMath)
    add_library(Bullet::LinearMath UNKNOWN IMPORTED)
    set_target_properties(Bullet::LinearMath PROPERTIES
            IMPORTED_LOCATION "${Bullet_LIBRARY}/libLinearMath.a"
            INTERFACE_INCLUDE_DIRECTORIES "${Bullet_INCLUDE_DIR}")
endif ()

if (Bullet_FOUND AND NOT TARGET Bullet::BulletInverseDynamics)
    add_library(Bullet::BulletInverseDynamics UNKNOWN IMPORTED)
    set_target_properties(Bullet::BulletInverseDynamics PROPERTIES
            IMPORTED_LOCATION "${Bullet_LIBRARY}/libBulletInveseDynamics.a"
            INTERFACE_INCLUDE_DIRECTORIES "${Bullet_INCLUDE_DIR}")
    target_link_libraries(Bullet::BulletInverseDynamics INTERFACE
            Bullet::LinearMath)
endif ()

if (Bullet_FOUND AND NOT TARGET Bullet::BulletCollision)
    add_library(Bullet::BulletCollision UNKNOWN IMPORTED)
    set_target_properties(Bullet::BulletCollision PROPERTIES
            IMPORTED_LOCATION "${Bullet_LIBRARY}/libBulletCollision.a"
            INTERFACE_INCLUDE_DIRECTORIES "${Bullet_INCLUDE_DIR}")
    target_link_libraries(Bullet::BulletCollision INTERFACE
            Bullet::LinearMath)
endif ()

if (Bullet_FOUND AND NOT TARGET Bullet::BulletDynamics)
    add_library(Bullet::BulletDynamics UNKNOWN IMPORTED)
    set_target_properties(Bullet::BulletDynamics PROPERTIES
            IMPORTED_LOCATION "${Bullet_LIBRARY}/libBulletDynamics.a"
            INTERFACE_INCLUDE_DIRECTORIES "${Bullet_INCLUDE_DIR}")
    target_link_libraries(Bullet::BulletDynamics INTERFACE
            Bullet::BulletCollision
            Bullet::LinearMath)
endif ()

if (Bullet_FOUND AND NOT TARGET Bullet::BulletSoftBody)
    add_library(Bullet::BulletSoftBody UNKNOWN IMPORTED)
    set_target_properties(Bullet::BulletSoftBody PROPERTIES
            IMPORTED_LOCATION "${Bullet_LIBRARY}/lib/BulletSoftBody.a"
            INTERFACE_INCLUDE_DIRECTORIES "${Bullet_INCLUDE_DIR}")
    target_link_libraries(Bullet::BulletSoftBody INTERFACE
            Bullet::LinearMath)
endif ()
