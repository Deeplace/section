<?php

function section_settings_form($form_state) {
  $form = array();

  return system_settings_form($form);
}

function section_view_form($form_state) {
  $form = array(
    '#tree' => TRUE,
    '#sections' => array(),
  );
  
  global $language;
  
  $sql = "SELECT s.*, sd.title FROM {sections} s
    LEFT JOIN {section_details} sd ON sd.section_id = s.id AND sd.language = '%s'
    ORDER BY s.weight ASC";
  $result = db_query($sql, $language->language);
  while ($section = db_fetch_object($result)) {
    $form['#sections'][$section->id] = $section;
    $form['weight'][$section->id] = array(
      '#type' => 'weight',
      '#delta' => 20,
      '#default_value' => $section->weight,
    );
  }
  
  if (count($form['#sections']) > 1) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
    );
  }
  else {
    unset($form['weight']);
  }
  
  return $form;
}

function section_view_form_submit($form, &$form_state) {
  foreach ($form_state['values']['weight'] as $id => $value) {
    if (is_numeric($id) && $value != $form['#sections'][$id]->weight) {
      db_query("UPDATE {sections} SET weight = %d WHERE id = %d", $value, $id);
    }
  }
}

function theme_section_view_form($form) {
  $output = '';

  $header = array(
    t('Machine name'),
    t('Table name'),
    t('Weight'),
    t('Type'),
    t('Status'),
    '',
    '',
  );
  $rows = array();
  $type = section_get_type_list();
  $status = section_get_status_list();

  foreach ($form['#sections'] as $section) {
    $title = empty($section->title) ? $section->machine_name : $section->title . ' (' . $section->machine_name . ')';
    if (isset($form['weight'][$section->id])) {
      $form['weight'][$section->id]['#attributes']['class'] = 'data-weight';
      $weight = drupal_render($form['weight'][$section->id]);
    }
    else {
      $weight = $section->weight;
    }
    
    $rows[] = array(
      'data' => array(
        check_plain($title),
        $section->table_name,
        $weight,
        empty($type[$section->type]) ? '' : $type[$section->type],
        empty($status[$section->status]) ? '' : $status[$section->status],
        l(t('Fields'), 'admin/build/section/' . $section->id . '/field'),
        l(t('Edit'), 'admin/build/section/' . $section->id . '/edit'),
      ),
      'class' => 'draggable',
    );
  }
  
  if (empty($rows)) {
    $rows[] = array(
      array(
        'data' => t('No items found.'),
        'colspan' => count($header),
      ),
    );
  }

  if (isset($form['submit'])) {
    drupal_add_tabledrag('section', 'order', 'sibling', 'data-weight');
  }
  
  $output .= theme('table', $header, $rows, array('id' => 'section'));
  $output .= drupal_render($form);

  return $output;
}

function section_edit_form($form_state, $section = NULL) {
  if (empty($section->id)) {
    $section = section_initialize();
  }
  else {
    drupal_set_title(t('Edit section <em>@name</em>', array('@name' => $section->machine_name)));
  }

  $form = array();
  $form['#section'] = $section;

  $form['machine_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Machine name'),
    '#required' => TRUE,
    '#default_value' => $section->machine_name,
  );

  $form['table_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Table name'),
    '#required' => TRUE,
    '#default_value' => $section->table_name,
  );

  $form['remote_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Remote key'),
    '#default_value' => $section->remote_key,
  );

  $languages = language_list('enabled');
  foreach ($languages[1] as $lang => $data) {
    $form['title_' . $lang] = array(
      '#type' => 'textfield',
      '#title' => t('Title (@lang)', array('@lang' => $lang)),
      '#default_value' => $section->details[$lang]['title'],
    );
  }

  $form['weight'] = array(
    '#type' => 'weight',
    '#title' => t('Weight'),
    '#delta' => 20,
    '#default_value' => $section->weight,
  );

  $form['type'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#options' => section_get_type_list(),
    '#default_value' => $section->type,
  );

  $form['status'] = array(
    '#type' => 'select',
    '#title' => t('Status'),
    '#options' => section_get_status_list(),
    '#default_value' => $section->status,
  );

  $form['validation'] = array(
    '#type' => 'textarea',
    '#title' => t('Validation'),
    '#rows' => 4,
    '#default_value' => $section->validation,
  );

  if ($section->locked) {
    $form['machine_name']['#disabled'] = TRUE;
    $form['table_name']['#disabled'] = TRUE;
    $form['remote_key']['#disabled'] = TRUE;
    $form['type']['#disabled'] = TRUE;
    $form['status']['#disabled'] = TRUE;
    $form['machine_name']['#required'] = FALSE;
    $form['table_name']['#required'] = FALSE;
    $form['remote_key']['#required'] = FALSE;
    $form['type']['#required'] = FALSE;
    $form['status']['#required'] = FALSE;
  }

  $form['actions'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );

  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  if (!empty($section->id)) {
    if (empty($section->locked)) {
      $form['actions']['delete'] = array(
        '#type' => 'submit',
        '#value' => t('Delete'),
      );
    }

    $form['actions']['cancel'] = array(
      '#value' => l(t('Cancel'), 'admin/build/section'),
    );
  }

  return $form;
}

function section_edit_form_validate($form, &$form_state) {
  $section = $form['#section'];

  if (!$section->locked) {
    if (!preg_match('|^[a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*$|', $form_state['values']['machine_name'])) {
      form_set_error('machine_name', t("Invalid name for machine name."));
    }

    $sql = "SELECT 1 FROM {sections} WHERE id <> %d AND machine_name = '%s'";
    if (db_result(db_query($sql, $section->id, $form_state['values']['machine_name']))) {
      form_set_error('machine_name', t('This machine name already exists.'));
    }

    if (empty($section->id) && db_table_exists($form_state['values']['table_name'])) {
      form_set_error('table_name', t('This table already exists.'));
    }

    if (!empty($form_state['values']['remote_key'])) {
      if (!preg_match('|^[a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*$|', $form_state['values']['remote_key'])) {
        form_set_error('machine_name', t("Invalid name for remote key."));
      }

      if (strtolower($form_state['values']['remote_key']) == 'id') {
        form_set_error('remote_key', t('Remote key must not be named as id.'));
      }
    }
  }
}

function section_edit_form_submit($form, &$form_state) {
  $section = $form['#section'];

  if ($form_state['values']['op'] == t('Delete')) {
    $form_state['redirect'] = 'admin/build/section/' . $section->id . '/delete';
    return;
  }

  $section->weight = $form_state['values']['weight'];

  if (!$section->locked) {
    $section->machine_name = $form_state['values']['machine_name'];
    $section->table_name = $form_state['values']['table_name'];
    $section->remote_key = $form_state['values']['remote_key'];
    $section->type = $form_state['values']['type'];
    $section->status = $form_state['values']['status'];
    $section->validation = $form_state['values']['validation'];
  }

  $languages = language_list('enabled');
  foreach ($languages[1] as $lang => $data) {
    $section->details[$lang]['title'] = $form_state['values']['title_' . $lang];
  }

  if (section_save($section)) {
    $attr = array(
      '@name' => $section->machine_name,
      '@id' => $section->id,
    );
    drupal_set_message(t('The section <em>@name</em> has been saved successfully', $attr));
    watchdog('section', 'Section @name (@id) saved', $attr);
    $form_state['redirect'] = array('admin/build/section');
  }
  else {
    $attr = array(
      '@name' => $section->machine_name,
      '@id' => $section->id,
      '@data' => print_r($section, TRUE),
    );
    drupal_set_message(t('Failed to save the section <em>@name</em>', $attr), 'error');
    watchdog('section', 'Section @name (@id) save failed. Data @data', $attr, WATCHDOG_ERROR);
  }
}

function section_delete_form($form_state, $section) {
  $form = array();
  $form['#section'] = $section;

  $question = t('Are you sure you want to delete the section <em>@name</em>?',
    array('@name' => $section->machine_name));
  $path = 'admin/build/section/' . $section->id . '/edit';

  return confirm_form($form, $question, $path);
}

function section_delete_form_submit($form, &$form_state) {
  $section = $form['#section'];

  if (section_delete($section)) {
    $attr = array(
      '@name' => $section->machine_name,
      '@id' => $section->id,
    );
    drupal_set_message(t('The section <em>@name</em> has been deleted successfully', $attr));
    watchdog('section', 'Section @name (@id) deleted', $attr);
    $form_state['redirect'] = 'admin/build/section';
  }
  else {
    $attr = array(
      '@name' => $section->machine_name,
      '@id' => $section->id,
    );
    drupal_set_message(t('Failed to delete the section <em>@name</em>', $attr), 'error');
    watchdog('section', 'Section @name (@id) delete failed', $attr, WATCHDOG_ERROR);
  }
}

function section_fields_form($form_state, $section) {
  $form = array(
    '#tree' => TRUE,
    '#fields' => array(),
    '#section' => $section,
  );
  
  $fields = section_load_fields($section);
  foreach ($fields as $field) {
    $form['#fields'][$field->id] = $field;
    $form['weight'][$field->id] = array(
      '#type' => 'weight',
      '#delta' => 20,
      '#default_value' => $field->weight,
    );
  }
  
  if (count($form['#fields']) > 1) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
    );
  }
  else {
    unset($form['weight']);
  }
  
  return $form;
}

function section_fields_form_submit($form, &$form_state) {
  foreach ($form_state['values']['weight'] as $id => $value) {
    if (is_numeric($id) && $value != $form['#fields'][$id]->weight) {
      db_query("UPDATE {section_fields} SET weight = %d WHERE id = %d", $value, $id);
    }
  }
}

function theme_section_fields_form($form) {
  $output = '';

  $header = array(
    t('Machine name'),
    t('Title'),
    t('Type'),
    t('Weight'),
    t('Searchable'),
    t('Status'),
  );
  $rows = array();
  $type = section_get_field_type_list();
  $status = section_get_status_list();
  $searchable = array(t('Yes'), t('No'));
  $section = $form['#section'];

  foreach ($form['#fields'] as $field) {
    if (isset($form['weight'][$field->id])) {
      $form['weight'][$field->id]['#attributes']['class'] = 'data-weight';
      $weight = drupal_render($form['weight'][$field->id]);
    }
    else {
      $weight = $field->weight;
    }
    
    $rows[] = array(
      'data' => array(
        l($field->machine_name, 'admin/build/section/' . $section->id . '/field/' . $field->id . '/edit'),
        $field->title,
        empty($type[$field->type]) ? '' : $type[$field->type],
        $weight,
        $searchable[(int) empty($field->searchable)],
        empty($status[$field->status]) ? '' : $status[$field->status],
      ),
      'class' => 'draggable',
    );
  }

  if (empty($rows)) {
    $rows[] = array(
      array('data' => t('No items found.'), 'colspan' => count($header)),
    );
  }

  if (isset($form['submit'])) {
    drupal_add_tabledrag('fields', 'order', 'sibling', 'data-weight');
  }
  
  $output .= theme('table', $header, $rows, array('id' => 'fields'));
  $output .= drupal_render($form);

  return $output;
}

function section_field_edit_form($form_state, $section, $field = NULL) {
  if (empty($field->id)) {
    $field = section_field_initialize($section);
  }
  else {
    if ($field->section_id != $section->id) {
      drupal_not_found();
      exit();
    }
    drupal_set_title(t('Edit field <em>@name</em>', array('@name' => $field->machine_name)));
  }
  
  drupal_add_js(drupal_get_path('module', 'section') . '/section.js');

  $form = array();
  $form['#section'] = $section;
  $form['#field'] = $field;

  $form['machine_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Machine name'),
    '#required' => TRUE,
    '#default_value' => $field->machine_name,
  );

  $languages = language_list('enabled');
  foreach ($languages[1] as $lang => $data) {
    $form['title_' . $lang] = array(
      '#type' => 'textfield',
      '#title' => t('Title (@lang)', array('@lang' => $lang)),
      '#default_value' => $field->details[$lang]['title'],
    );
  }

  foreach ($languages[1] as $lang => $data) {
    $form['description_' . $lang] = array(
      '#type' => 'textfield',
      '#title' => t('Description (@lang)', array('@lang' => $lang)),
      '#default_value' => $field->details[$lang]['description'],
    );
  }

  $form['category'] = array(
    '#type' => 'textfield',
    '#title' => t('Category'),
    '#default_value' => $field->category,
  );

  $form['type'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#options' => section_get_field_type_list(),
    '#default_value' => $field->type,
  );

  $sql_type = section_build_sql_spec($field);
  $form['sql_type'] = array(
    '#type' => 'fieldset',
    '#title' => t('SQL type'),
    '#collapsible' => TRUE,
    //'#collapsed' => !empty($field->sql_type),
    '#tree' => TRUE,
  );
  
  $form['sql_type']['type'] = array(
    '#type' => 'select',
    '#title' => t('Field type'),
    '#options' => drupal_map_assoc(array(
      'serial',
      'int',
      'float',
      'numeric',
      'varchar',
      'char',
      'text',
      'blob',
      'datetime',
    )),
    '#default_value' => isset($sql_type['type']) ? $sql_type['type'] : 'varchar',
  );

  $form['sql_type']['length'] = array(
    '#type' => 'textfield',
    '#title' => t('Length'),
    '#size' => 20,
    '#default_value' => isset($sql_type['length']) ? $sql_type['length'] : '',
  );

  $form['sql_type']['not null'] = array(
    '#type' => 'checkbox',
    '#title' => t('Not null'),
    '#default_value' => !empty($sql_type['not null']),
  );

  $form['sql_type']['unsigned'] = array(
    '#type' => 'checkbox',
    '#title' => t('Unsigned'),
    '#default_value' => !empty($sql_type['unsigned']),
  );

  $form['sql_type']['precission'] = array(
    '#type' => 'textfield',
    '#title' => t('Precission'),
    '#size' => 20,
    '#default_value' => isset($sql_type['precission']) ? $sql_type['precission'] : '',
  );

  $form['sql_type']['scale'] = array(
    '#type' => 'textfield',
    '#title' => t('Scale'),
    '#size' => 20,
    '#default_value' => isset($sql_type['scale']) ? $sql_type['scale'] : '',
  );

  $form['sql_type']['default'] = array(
    '#type' => 'textfield',
    '#title' => t('Default'),
    '#size' => 50,
    '#default_value' => isset($sql_type['default']) ? $sql_type['default'] : '',
  );

  $form['required'] = array(
    '#type' => 'checkbox',
    '#title' => t('Required'),
    '#default_value' => $field->required,
  );

  $form['searchable'] = array(
    '#type' => 'select',
    '#title' => t('Searchable'),
    '#options' => array('' => t('Not searchable')) + section_get_search_conditions_types(),
    '#default_value' => $field->searchable,
  );

  $form['weight'] = array(
    '#type' => 'weight',
    '#title' => t('Weight'),
    '#delta' => 20,
    '#default_value' => $field->weight,
  );

  $form['default_value'] = array(
    '#type' => 'textfield',
    '#title' => t('Default value'),
    '#default_value' => $field->default_value,
  );

  $form['status'] = array(
    '#type' => 'select',
    '#title' => t('Status'),
    '#options' => section_get_status_list(),
    '#default_value' => $field->status,
  );

  $form['field_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Field size'),
    '#default_value' => $field->field_size,
  );

  $form['min_length'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimal length'),
    '#default_value' => $field->min_length,
  );

  $form['max_length'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximal length'),
    '#default_value' => $field->max_length,
  );

  $form['file_extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Allowed extensions'),
    '#description' => t('Comma separated values'),
    '#default_value' => $field->file_extensions,
  );

  $form['file_max_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximal file size'),
    '#default_value' => $field->file_max_size,
  );

  $form['file_upload_path'] = array(
    '#type' => 'textfield',
    '#title' => t('File upload path'),
    '#default_value' => $field->file_upload_path,
  );

  $list = array('' => t('- None -'));
  if (module_exists('imagecache')) {
    $presets = imagecache_presets();
    foreach ($presets as $preset) {
      $list[$preset['presetname']] = $preset['presetname'];
    }
  }
  $form['imagecache_preset'] = array(
    '#type' => 'select',
    '#title' => t('ImageCache preset'),
    '#options' => $list,
    '#default_value' => $field->imagecache_preset,
  );

  $form['source'] = array(
    '#type' => 'textarea',
    '#title' => t('Source'),
    '#default_value' => $field->source,
  );

  if ($field->locked) {
    foreach ($form['sql_type'] as $key => $element) {
      if (substr($key, 0, 1) != '#') {
        $form['sql_type'][$key]['#disabled'] = TRUE;
        $form['sql_type'][$key]['#required'] = FALSE;
      }
    }
    $form['machine_name']['#disabled'] = TRUE;
    $form['machine_name']['#required'] = FALSE;
    $form['status']['#disabled'] = TRUE;
    $form['status']['#required'] = FALSE;
  }

  $form['actions'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );

  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  if (!empty($field->id)) {
    if (empty($section->locked)) {
      $form['actions']['delete'] = array(
        '#type' => 'submit',
        '#value' => t('Delete'),
      );
    }

    $form['actions']['cancel'] = array(
      '#value' => l(t('Cancel'), 'admin/build/section/' . $section->id . '/field'),
    );
  }

  return $form;
}

function section_field_edit_form_validate($form, &$form_state) {
  $section = $form['#section'];
  $field = $form['#field'];

  if (!$field->locked) {
    if (!preg_match('|^[a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*$|', $form_state['values']['machine_name'])) {
      form_set_error('machine_name', t("Invalid name for machine name."));
    }

    if (in_array(strtolower($form_state['values']['machine_name']), array('id', strtolower($section->remote_key)))) {
      form_set_error('machine_name', t("Machine name must not be named as id or as section's remote key."));
    }
  }
}

function section_field_edit_form_submit($form, &$form_state) {
  $section = $form['#section'];
  $field = $form['#field'];

  if ($form_state['values']['op'] == t('Delete')) {
    $form_state['redirect'] = 'admin/build/section/' . $section->id . '/field/' . $field->id . '/delete';
    return;
  }

  $field->type = $form_state['values']['type'];
  $field->searchable = $form_state['values']['searchable'];
  $field->weight = $form_state['values']['weight'];
  $field->searchable = $form_state['values']['searchable'];
  $field->category = $form_state['values']['category'];
  $field->field_size = $form_state['values']['field_size'];
  $field->min_length = $form_state['values']['min_length'];
  $field->max_length = $form_state['values']['max_length'];
  $field->required = $form_state['values']['required'];
  $field->default_value = $form_state['values']['default_value'];
  $field->source = $form_state['values']['source'];
  $field->file_extensions = $form_state['values']['file_extensions'];
  $field->file_max_size = $form_state['values']['file_max_size'];
  $field->file_upload_path = $form_state['values']['file_upload_path'];
  $field->imagecache_preset = $form_state['values']['imagecache_preset'];

  if (!$field->locked) {
    $field->machine_name = $form_state['values']['machine_name'];
    $field->sql_type = section_build_sql_type($form_state['values']['sql_type']);
    $field->status = $form_state['values']['status'];
  }

  $languages = language_list('enabled');
  foreach ($languages[1] as $lang => $data) {
    $field->details[$lang]['title'] = $form_state['values']['title_' . $lang];
    $field->details[$lang]['description'] = $form_state['values']['description_' . $lang];
  }

  if (section_field_save($field)) {
    $attr = array(
      '@name' => $field->machine_name,
      '@id' => $field->id,
    );
    drupal_set_message(t('The field <em>@name</em> has been saved successfully', $attr));
    watchdog('section', 'Field @name (@id) saved', $attr);
    $form_state['redirect'] = array('admin/build/section/' . $section->id . '/field');
  }
  else {
    $attr = array(
      '@name' => $field->machine_name,
      '@id' => $field->id,
      '@data' => print_r($field, TRUE),
    );
    drupal_set_message(t('Failed to save the field <em>@name</em>', $attr), 'error');
    watchdog('section', 'Field @name (@id) save failed. Data @data', $attr, WATCHDOG_ERROR);
  }
}

function section_field_delete_form($form_state, $section, $field) {
  if ($field->section_id != $section->id) {
    drupal_not_found();
    exit();
  }

  $form = array();
  $form['#section'] = $section;
  $form['#field'] = $field;

  $question = t('Are you sure you want to delete the field <em>@name</em>?',
    array('@name' => $field->machine_name));
  $path = 'admin/build/section/' . $section->id . '/field/' . $field->id . '/edit';

  return confirm_form($form, $question, $path);
}

function section_field_delete_form_submit($form, &$form_state) {
  $section = $form['#section'];
  $field = $form['#field'];

  if (section_field_delete($field)) {
    $attr = array(
      '@name' => $field->machine_name,
      '@id' => $field->id,
    );
    drupal_set_message(t('The field <em>@name</em> has been deleted successfully', $attr));
    watchdog('section', 'Field @name (@id) deleted', $attr);
    $form_state['redirect'] = 'admin/build/section/' . $section->id . '/field';
  }
  else {
    $attr = array(
      '@name' => $field->machine_name,
      '@id' => $field->id,
    );
    drupal_set_message(t('Failed to delete the section <em>@name</em>', $attr), 'error');
    watchdog('section', 'Section @name (@id) delete failed', $attr, WATCHDOG_ERROR);
  }
}

function section_build_sql_type($array) {
  $sql_type = array();
  
  $sql_type[] = 'type:' . $array['type'];
  if ($array['type'] == 'datetime') {
    $sql_type[] = 'mysql_type:date';
  }
  
  if (!empty($array['length'])) {
    $sql_type[] = 'length:' . $array['length'];
  }
  
  if (!empty($array['unsigned'])) {
    $sql_type[] = 'unsigned:1';
  }
  
  if (!empty($array['not null'])) {
    $sql_type[] = 'not null:1';
  }
  
  if (!empty($array['precission'])) {
    $sql_type[] = 'precission:' . $array['precission'];
  }
  
  if (!empty($array['scale'])) {
    $sql_type[] = 'scale:' . $array['scale'];
  }
  
  if (strlen($array['default'])) {
    $sql_type[] = 'default:' . $array['default'];
  }
  
  // TODO: Do some validations here
  
  return implode(';', $sql_type);
}
