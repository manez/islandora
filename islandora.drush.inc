<?php

/**
 * @file
 * Drush command/hook implementation.
 */

/**
 * Implements hook_drush_command().
 */
function islandora_drush_command() {
  $commands = array();

  $commands['islandora-solution-pack-install-required-objects'] = array(
    'description' => dt('Install Solution Pack objects.'),
    'options' => array(
      'module' => array(
        'description' => dt('The module for which to install the required objects.'),
        'required' => TRUE,
      ),
      'force' => array(
        'description' => dt('Force reinstallation of the objects.'),
      ),
    ),
    'aliases' => array('ispiro'),
    'drupal dependencies' => array(
      'islandora',
    ),
    'examples' => array(
      'drush -u 1 ispiro --module=islandora' => dt('Install missing solution pack objects for the "islandora" module.'),
      'drush -u 1 ispiro --module=islandora --force' => dt('Install all solution pack objects for the "islandora" module, purging any which currently exist.'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  $commands['islandora-solution-pack-uninstall-required-objects'] = array(
    'description' => dt('Uninstall Solution Pack objects.'),
    'options' => array(
      'module' => array(
        'description' => dt('The module for which to uninstall the required objects.'),
        'required' => TRUE,
      ),
      'force' => array(
        'description' => dt('Force uninstallation of the objects.'),
      ),
    ),
    'aliases' => array('ispuro'),
    'drupal dependencies' => array(
      'islandora',
    ),
    'examples' => array(
      'drush -u 1 ispuro --module=islandora' => dt('Uninstall solution pack objects for the "islandora" module.'),
      'drush -u 1 ispuro --module=islandora --force' => dt('Force uninstallation of all solution pack objects for the "islandora" module.'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  $commands['islandora-solution-pack-required-objects-status'] = array(
    'description' => dt('Get Solution Pack object status.'),
    'options' => array(
      'module' => array(
        'description' => dt('The module for which to get the status of the required objects.'),
      ),
    ),
    'aliases' => array('ispros'),
    'drupal dependencies' => array(
      'islandora',
    ),
    'examples' => array(
      'drush -u 1 ispros' => dt('Get the status of all solution pack objects.'),
      'drush -u 1 ispros --module=islandora' => dt('Get the status of solution pack objects for the "islandora" module.'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  $commands['islandora-solution-pack-install-content_models'] = array(
    'description' => dt('Install Solution Pack content models.'),
    'options' => array(
      'module' => array(
        'description' => dt('The module for which to install the content models.'),
        'required' => TRUE,
      ),
    ),
    'aliases' => array('ispicm'),
    'drupal dependencies' => array(
      'islandora',
    ),
    'examples' => array(
      'drush -u 1 ispicm --module=islandora' => dt('Install missing solution pack objects for the "islandora" module.'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  return $commands;
}

/**
 * Command callback to install required objects.
 */
function drush_islandora_solution_pack_install_required_objects() {
  module_load_include('inc', 'islandora', 'includes/solution_packs');

  $module = drush_get_option('module');
  if (module_exists($module)) {
    islandora_install_solution_pack(
        $module, 'install', drush_get_option('force', FALSE)
    );
  }
  else {
    drush_log(dt('"@module" is not installed/enabled?...', array(
      '@module' => $module,
    )));
  }
}

/**
 * Command callback to uninstall required objects.
 */
function drush_islandora_solution_pack_uninstall_required_objects() {
  module_load_include('inc', 'islandora', 'includes/solution_packs');

  $module = drush_get_option('module');
  if (module_exists($module)) {
    islandora_uninstall_solution_pack(
        $module, drush_get_option('force', FALSE)
    );
  }
  else {
    drush_log(dt('"@module" is not installed/enabled?...', array(
      '@module' => $module,
    )));
  }
}

/**
 * Command callback for object status.
 */
function drush_islandora_solution_pack_required_objects_status() {
  module_load_include('inc', 'islandora', 'includes/solution_packs');

  $module = drush_get_option('module', FALSE);
  $required_objects = array();
  if ($module && module_exists($module)) {
    $required_objects[$module] = islandora_solution_packs_get_required_objects($module);
  }
  elseif ($module === FALSE) {
    $required_objects = islandora_solution_packs_get_required_objects();
  }
  else {
    drush_log(dt('"@module" is not installed/enabled?...', array(
      '@module' => $module,
    )));
    return;
  }

  $header = array('PID', 'Machine Status', 'Readable Status');
  $widths = array(30, 20, 20);
  foreach ($required_objects as $module => $info) {
    $rows = array();
    drush_print($info['title']);
    foreach ($info['objects'] as $object) {
      $status = islandora_check_object_status($object);
      $rows[] = array(
        $object->id,
        $status['status'],
        $status['status_friendly'],
      );
    }
    drush_print_table($rows, $header, $widths);
  }
}

/**
 * Command callback to install required objects.
 */
function drush_islandora_solution_pack_install_content_models() {
  module_load_include('inc', 'islandora', 'includes/solution_packs');
  $module = drush_get_option('module');
  if (module_exists($module)) {
    $info = islandora_solution_packs_get_required_objects($module);
    $objects_to_add = array();
    foreach ($info['objects'] as $key => $candidate) {
      if (in_array('fedora-system:ContentModel-3.0', $candidate->models)) {
        $objects_to_add[] = $candidate;
      }
    }
    if (count($objects_to_add) > 0) {
      foreach ($objects_to_add as $object_to_add) {
        $old_object = islandora_object_load($object_to_add->id);
        if ($old_object) {
          $deleted = islandora_delete_object($old_object);
          if (!$deleted) {
            drush_log(dt('@object did not delete.', array('@object' => $old_object->id), 'error'));
            continue;
          }
        }
        $new_object = islandora_add_object($object_to_add);
        $verb = $deleted ? dt("Replaced") : dt("Added");
        if ($new_object) {
          drush_print("$verb " . $object_to_add->id . " - " . $object_to_add->label);
        }
      }
    }
  }
}
