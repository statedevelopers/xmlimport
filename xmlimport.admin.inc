<?php

/**
 * @file
 * The admin settings for the XML Import module
 */

/**
 * Implementation of hook_settings
 */
function xmlimport_admin_settings() {
  $form['xmlimport_header'] = array(
    '#markup' => '<p>Be sure to include in your XML file, tags for "entity_type" and "bundle" for each entity.</p>' . 
    '<p><em>To execute the import, go to the ' . 
    '<a href="/xmlimport/run/' . variable_get('xmlimport_file', '') . 
    '/' . variable_get('xmlimport_offset_iterations', 0) . 
    '/' . variable_get('xmlimport_max_iterations', 1000) . 
    '">"Run" page</a>.<hr /></em></p>',    
  );
  $form['xmlimport_file'] = array(
    '#type' => 'textfield',
    '#title' => t('File path'),
    '#required' => TRUE,
    '#default_value' => variable_get('xmlimport_file', ''),
    '#description' => t('Enter the path to the XML file you want to import.<br />NOTE: the file entities should be in standard phpMyAdmin format, enclosed in a table tag within a database tag.'),
  );

  $form['xmlimport_savenew'] = array(
    '#type' => 'checkbox',
    '#title' => t('Create new entities.'),
    '#required' => FALSE,
    '#default_value' => variable_get('xmlimport_savenew', 0),
    '#description' => t('Create entity if it is not found in the system.'),
  );
  $form['xmlimport_saveexisting'] = array(
    '#type' => 'checkbox',
    '#title' => t('Save changes to existing.'),
    '#required' => FALSE,
    '#default_value' => variable_get('xmlimport_saveexisting', 0),
    '#description' => t('Save changes to existing entities that match the data.'),
  );
  $form['xmlimport_offset_iterations'] = array(
    '#type' => 'textfield',
    '#title' => t('Offset iterations'),
    '#required' => TRUE,
    '#default_value' => variable_get('xmlimport_offset_iterations', 0),
    '#description' => t('If you would like to start in the middle of the file, enter the index where to start.'),
  );
  $form['xmlimport_max_iterations'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum iterations'),
    '#required' => TRUE,
    '#default_value' => variable_get('xmlimport_max_iterations', 1000),
    '#description' => t('Enter the maximum number of items to import at once. Enter zero for no limit.'),
  );

  return system_settings_form($form);
}

function xmlimport_admin_settings_validate($form, &$form_state) {
}

function xmlimport_admin_settings_submit($form, &$form_state) {
  // clean up the data ...

  // ... and save the settings
  system_settings_form_submit($form, $form_state);
}

