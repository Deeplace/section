<?php

define('SECTION_TYPE_ONE_TO_MANY', 0);
define('SECTION_TYPE_ONE_TO_ONE', 1);
define('SECTION_STATUS_DISABLED', 0);
define('SECTION_STATUS_ENABLED', 1);

function section_initialize() {
  $section = (object) array(
    'machine_name' => '',
    'table_name' => '',
    'remote_key' => '',
    'weight' => 0,
    'type' => SECTION_TYPE_ONE_TO_MANY,
    'status' => SECTION_STATUS_ENABLED,
    'details' => array(),
  );
  
  $languages = language_list('enabled');
  foreach ($languages[1] as $lang => $data) {
    $section->details[$lang] = array(
      'language' => $lang,
      'title' => '',
    );
  }
  
  return $section;
}

function section_load($id) {
  static $sections = array();
  
  if (!isset($sections[$id])) {
    if (is_numeric($id)) {
      $sql = "SELECT * FROM {sections} WHERE id = %d";
    }
    else {
      $sql = "SELECT * FROM {sections} WHERE machine_name = '%s'";
    }
    $section = db_fetch_object(db_query($sql, $id));
    if (!empty($section)) {
      $section->_table_name = $section->table_name;
      $section->_type = $section->type;
      $section->_remote_key = $section->remote_key;
      $section->details = array();
      
      $languages = language_list('enabled');
      foreach ($languages[1] as $lang => $data) {
        $section->details[$lang] = array(
          'language' => $lang,
          'title' => '',
        );
      }
      
      $result = db_query("SELECT * FROM {section_details} WHERE section_id = %d", $section->id);
      while ($data = db_fetch_array($result)) {
        $section->details[$data['language']] = $data;
      }
      
      $sections[$section->id] = $section;
      $sections[$section->machine_name] = $section;
    }
    else {
      $sections[$id] = $section;
    }
  }
  
  return $sections[$id];
}

function section_save($section) {
  $create = empty($section->id) || (!empty($section->_table_name) && !db_table_exists($section->_table_name));
  
  db_query("START TRANSACTION");

  if (drupal_write_record('sections', $section, $create ? array() : 'id') === FALSE) {
    db_query("ROLLBACK");
    return FALSE;
  }

  foreach ($section->details as $lang => $data) {
    $data['section_id'] = $section->id;
    if (drupal_write_record('section_details', $data, empty($data['id']) ? array() : 'id') === FALSE) {
      db_query("ROLLBACK");
      return FALSE;
    }
    
    $section->details[$lang] = $data;
  }

  $ret = array();
  if ($create) {
    $schema = array();
    $schema[$section->table_name] = array(
      'fields' => array(
        'id' => array('type' => 'serial', 'unsigned' => TRUE, 'not null' => TRUE),
      ),
      'primary key' => array('id'),
    );
    _drupal_initialize_schema('section', $schema);
    foreach ($schema as $name => $table) {
      db_create_table($ret, $name, $table);
    }
  }

  if (!empty($section->_table_name) && $section->_table_name != $section->table_name) {
    db_rename_table($ret, $section->_table_name, $section->table_name);
  }

  if (db_table_exists($section->table_name)) {
    if (empty($section->_remote_key) && !empty($section->remote_key)) {
      $keys = array(
        'indexes' => array(
          $section->remote_key . '_key' => array($section->remote_key)
        ),
      );
      $spec = array('type' => 'int', 'unsigned' => TRUE, 'not null' => TRUE);
      db_add_field($ret, $section->table_name, $section->remote_key, $spec, $keys);
    }
    elseif (!empty($section->_remote_key) && empty($section->remote_key)) {
      db_drop_field($ret, $section->table_name, $section->remote_key);
    }
    elseif ($section->_remote_key != $section->remote_key) {
      $spec = array('type' => 'int', 'unsigned' => TRUE, 'not null' => TRUE);
      db_change_field($ret, $section->table_name, $section->_remote_key, $section->remote_key, $spec);
    }
  }
  
  foreach ($ret as $item) {
    if (!$item['success']) {
      db_query("ROLLBACK");
      return FALSE;
    }
  }
  
  db_query("COMMIT");

  if (!empty($ret)) {
    drupal_get_schema(FALSE, TRUE);
  }
  
  return TRUE;
}

function section_delete($section) {
  $sql = "DELETE FROM {section_field_details}
    WHERE field_id IN (SELECT id FROM {section_fields} WHERE section_id = %d)";
  db_query($sql, $section->id);
  db_query("DELETE FROM {section_fields} WHERE section_id = %d", $section->id);
  db_query("DELETE FROM {section_details} WHERE section_id = %d", $section->id);
  db_query("DELETE FROM {sections} WHERE id = %d", $section->id);
  if (db_table_exists($section->table_name)) {
    $ret = array();
    db_drop_table($ret, $section->table_name);
  }
  drupal_get_schema(FALSE, TRUE);
  
  return TRUE;
}

function section_field_initialize($section) {
  $field = (object) array(
    'section_id' => $section->id,
    'machine_name' => '',
    'type' => SECTION_FIELD_TYPE_TEXTFIELD,
    'searchable' => 0,
    'weight' => 0,
    'status' => SECTION_STATUS_ENABLED,
    'details' => array(),
  );
  
  $languages = language_list('enabled');
  foreach ($languages[1] as $lang => $data) {
    $field->details[$lang] = array(
      'language' => $lang,
      'title' => '',
      'description' => '',
    );
  }
  
  return $field;
}

function section_field_load($id) {
  static $fields = array();
  
  if (!isset($fields[$id])) {
    $sql = "SELECT * FROM {section_fields} WHERE id = %d";
    $fields[$id] = db_fetch_object(db_query($sql, $id));
    if (!empty($fields[$id])) {
      $fields[$id]->_machine_name = $fields[$id]->machine_name;
      $fields[$id]->_sql_type = $fields[$id]->sql_type;
      $fields[$id]->details = array();
      
      $languages = language_list('enabled');
      foreach ($languages[1] as $lang => $data) {
        $fields[$id]->details[$lang] = array(
          'language' => $lang,
          'title' => '',
          'description' => '',
        );
      }
      
      $result = db_query("SELECT * FROM {section_field_details} WHERE field_id = %d", $id);
      while ($data = db_fetch_array($result)) {
        $fields[$id]->details[$data['language']] = $data;
      }
    }
  }
  
  return $fields[$id];
}

function section_field_save($field) {
  $create = empty($field->id);
  
  db_query("START TRANSACTION");
  
  if (drupal_write_record('section_fields', $field, $create ? array() : 'id') === FALSE) {
    db_query("ROLLBACK");
    return FALSE;
  }
  
  foreach ($field->details as $lang => $data) {
    $data['field_id'] = $field->id;
    
    if (drupal_write_record('section_field_details', $data, empty($data['id']) ? array() : 'id') === FALSE) {
      db_query("ROLLBACK");
      return FALSE;
    }
    
    $field->details[$lang] = $data;
  }
  
  $section = section_load($field->section_id);
  
  $ret = array();
  if ($create) {
    $spec = section_build_sql_spec($field);
    if (empty($spec)) {
      $ret[] = array('succes' => FALSE);
    }
    else {
      db_add_field($ret, $section->table_name, $field->machine_name, $spec);
    }
  }
  else {
    if ((!empty($field->_machine_name) && $field->_machine_name != $field->machine_name) ||
      (!empty($field->_sql_type) && $field->_sql_type != $field->sql_type)) {
        $spec = section_build_sql_spec($field);
        if (empty($spec)) {
          $ret[] = array('succes' => FALSE);
        }
        else {
          db_change_field($ret, $section->table_name, $field->_machine_name, $field->machine_name, $spec);
        }
    }
  }
  
  foreach ($ret as $item) {
    if (!$item['success']) {
      db_query("ROLLBACK");
      return FALSE;
    }
  }
  
  db_query("COMMIT");
  
  if (!empty($ret)) {
    drupal_get_schema(FALSE, TRUE);
  }
  
  return TRUE;
}

function section_field_delete($field) {
  $section = section_load($field->section_id);
  
  db_query("DELETE FROM {section_field_details} WHERE field_id = %d", $field->id);
  db_query("DELETE FROM {section_fields} WHERE id = %d", $field->id);
  $ret = array();
  db_drop_field($ret, $section->table_name, $field->machine_name);
  drupal_get_schema(FALSE, TRUE);
  
  return TRUE;
}

function section_build_sql_spec($field) {
  $spec = array();
  $temp = explode(';', $field->sql_type);
  foreach ($temp as $value) {
    list($key, $type) = array_map('trim', explode(':', $value));
    if (!empty($key) && !empty($type)) {
      $spec[$key] = $type;
    }
  }
  
  if (empty($spec['type']) && empty($spec['mysql_type'])) {
    return array();
  }
  
  if (empty($spec['length']) && ($spec['type'] == 'varchar' || $spec['type'] == 'char')) {
    $spec['length'] = $field->max_length;
  }
  
  return $spec;
}

function section_load_sections($enabled_only = FALSE, $reset = FALSE) {
  static $sections;

  if (!isset($sections) || $reset) {
    $sql = "SELECT * FROM {sections}";
    $values = array();
    if ($enabled_only) {
      $sql .= " WHERE status = %d";
      $values[] = SECTION_STATUS_ENABLED;
    }
    $sql .= " ORDER BY weight ASC";
    $result = db_query($sql, $values);
    while ($data = db_fetch_object($result)) {
      $sections[$data->machine_name] = $data;
    }
  }
  
  return $sections;
}

function section_load_fields($section, $enabled_only = FALSE, $reset = FALSE) {
  static $fields = array();
  global $language;

  if (!isset($fields[$section->id]) || $reset) {
    $fields[$section->id] = array();
    $sql = "SELECT f.*, fd.title, fd.description FROM {section_fields} f
      LEFT JOIN {section_field_details} fd ON fd.field_id = f.id AND fd.language = '%s'
      WHERE f.section_id = %d";
    $values = array($language->language, $section->id);
    if ($enabled_only) {
      $sql .= " AND status = %d";
      $values[] = SECTION_STATUS_ENABLED;
    }
    $sql .= " ORDER BY weight ASC";
    $result = db_query($sql, $values);
    while ($data = db_fetch_object($result)) {
      $fields[$section->id][$data->machine_name] = $data;
    }
  }

  return $fields[$section->id];
}

function section_entity_load($section, $entity_id, $record_id = 0, $order_by = '', $sort = 'asc') {
  $data = FALSE;
  
  if (empty($section->remote_key)) {
    $sql = "SELECT * FROM {" . $section->table_name . "} WHERE id = %d";
    $data = db_fetch_object(db_query($sql, $entity_id));
  }
  elseif ($section->type == SECTION_TYPE_ONE_TO_MANY) {
    if (empty($record_id)) {
      $data = array();
      $sql = "SELECT * FROM {" . $section->table_name . "} WHERE `" . $section->remote_key . "` = %d";
      if (!empty($order_by)) {
        $sql .= " ORDER BY " . $order_by . ' ' . $sort;
      }
      $result = db_query($sql, $entity_id);
      while ($d = db_fetch_object($result)) {
        $data[] = $d;
      }
    }
    else {
      $sql = "SELECT * FROM {" . $section->table_name . "} WHERE `" . $section->remote_key . "` = %d AND `id` = %d LIMIT 1";
      $data = db_fetch_object(db_query($sql, $entity_id, $record_id));
    }
  }
  else {
    $sql = "SELECT * FROM {" . $section->table_name . "} WHERE `" . $section->remote_key . "` = %d ORDER BY `id` ASC LIMIT 1";
    $data = db_fetch_object(db_query($sql, $entity_id));
  }
  
  if (!empty($data)) {
    foreach ($data as $key => $value) {
      if (is_object($value)) {
        foreach ($value as $k => $v) {
          if (strpos($v, '0000-00-00') !== FALSE) {
            $data[$key]->$k = 0;
          }
        }
      }
      else {
        if (strpos($value, '0000-00-00') !== FALSE) {
          $data->$key = 0;
        }
      }
    }
  }

  return $data;
}

function section_entity_save($section, $entity) {
  $update = empty($entity->id) ? array() : 'id';
  return drupal_write_record($section->table_name, $entity, $update) !== FALSE;
}

function section_entity_delete($section, $entity) {
  db_query("DELETE FROM {" . $section->table_name . "} WHERE id = %d", $entity->id);
  
  return TRUE;
}

function section_get_type_list() {
  return array(
    SECTION_TYPE_ONE_TO_MANY => t('One to many'),
    SECTION_TYPE_ONE_TO_ONE => t('One to one'),
  );
}

function section_get_field_type_list() {
  return array(
    'textfield'  => t('Textfield'),
    'textarea'   => t('Textarea'),
    'date'       => t('Date'),
    'timestamp'  => t('Timestamp'),
    'select'     => t('Select'),
    'checkboxes' => t('Checkboxes'),
    'radios'     => t('Radios'),
    'checkbox'   => t('Checkbox'),
    'hidden'     => t('Hidden'),
    'file'       => t('File'),
    'image'      => t('Image'),
  );
}

function section_get_status_list() {
  return array(
    SECTION_STATUS_DISABLED => t('Disabled'),
    SECTION_STATUS_ENABLED => t('Enabled'),
  );
}

function section_get_search_conditions_types() {
  $list = array();
  $result = db_query("SELECT DISTINCT category FROM {search_conditions}");
  while ($data = db_result($result)) {
    $list[$data] = $data;
  }
  return $list;
}

function section_build_field_options($source, $required = FALSE, $placeholders = array()) {
  static $options = array();
  
  $source = trim(strtr($source, $placeholders));
  $key = md5($source);
  
  if (!isset($options[$key])) {
    $options[$key] = array();
    $source_onerow = trim(strtr($source, array("\r" => '', "\n" => " ")));
    
    if (strpos($source_onerow, 'taxonomy:') === 0) {
      $vid = trim(str_replace('taxonomy:', '', $source_onerow));
      if (!is_numeric($vid)) {
        $vid = taxonomy_ext_get_vid_by_machine_name($vid);
      }
      
      if (!empty($vid)) {
        $sql = "SELECT tid, name FROM {term_data} WHERE vid = %d";
        $result = db_query($sql, $vid);
        while ($data = db_fetch_object($result)) {
          $options[$key][$data->tid] = $data->name;
        }
      }
    }
    elseif (preg_match('/^.*SELECT.+FROM.+$/i', $source_onerow)) {
      $result = db_query($source);
      while ($data = db_fetch_array($result)) {
        $data = array_map('trim', array_values($data));
        $id = array_shift($data);
        $options[$key][$id] = implode(' ', $data);
      }
    }
    elseif (preg_match("|<\?php(.*) \?>|i", $source_onerow, $match)) {
      $options[$key] = eval($match[1]);
      if (empty($options[$key])) {
        $options[$key] = array();
      }
    }
    else {
      $str = array_values(array_map('trim', explode("\n", $source)));
      foreach ($str as $line) {
        $line = trim($line);
        $arr = explode('|', $line, 2);
        if (count($arr) > 1) {
          $options[$key][trim($arr[0])] = trim($arr[1]);
        }
        else {
          $options[$key][$line] = $line;
        }
      }
    }
  }
  
  if (!$required) {
    return array('' => t('- None -')) + $options[$key];
  }
  
  return $options[$key];
}

function section_format_value($field, $value, $placeholders = array()) {
  $value = isset($value) ? $value : $field->default_value;
  switch ($field->type) {
    case 'date':
    case 'timestamp':
      if (is_numeric($value)) {
        $value = empty($value) ? '' : date('d.m.Y', $value);
      }
      else {
        if ($value == '0000-00-00') {
          $value = '';
        }
        else {
          $value = date('d.m.Y', strtotime($value));
        }
      }
      break;
      
    case 'select':
    case 'checkboxes':
    case 'radios':
      $options = section_build_field_options($field->source, $field->required, $placeholders);
      if (!empty($options[$value])) {
        $value = $options[$value];
      }
      break;
      
    case 'file':
      $file = db_fetch_object(db_query("SELECT * FROM {files} WHERE fid = %d", $value));
      if (!empty($file)) {
        $lang = language_default();
        $value = l($file->filename, $file->filepath, array('language' => $lang));
      }
      else {
        $value = '';
      }
      break;
      
    case 'image':
      $file = db_fetch_object(db_query("SELECT * FROM {files} WHERE fid = %d", $value));
      if (!empty($file)) {
        $lang = language_default();
        $value = l($file->filename, $file->filepath, array('language' => $lang));
      }
      else {
        $value = '';
      }
      break;
  }
  
  return $value;
}

function section_build_form(&$form_state, $section, $entity_id = 0, $record_id = 0) {
  if (empty($section)) {
    drupal_not_found();
    exit();
  }
  
  if (empty($section->remote_key)) {
    if (empty($entity_id)) {
      $entity = (object) array(
        'id' => 0,
      );
    }
    else {
      $entity = section_entity_load($section, $entity_id);
    }
  }
  else {
    if (!empty($entity_id)) {
      if ($section->type == SECTION_TYPE_ONE_TO_MANY) {
        if (empty($record_id)) {
          $entity = (object) array(
            'id' => 0,
            $section->remote_key => $entity_id,
          );
        }
        else {
          $entity = section_entity_load($section, $entity_id, $record_id);
        }
      }
      else {
        $entity = section_entity_load($section, $entity_id);
        if (empty($entity)) {
          $entity = (object) array(
            'id' => 0,
            $section->remote_key => $entity_id,
          );
        }
      }
    }
  }
  
  if (empty($entity)) {
    drupal_not_found();
    exit();
  }
  
  $form = array();
  $form['#entity_id'] = $entity_id;
  $form['#record_id'] = $record_id;
  $form['#section'] = $section;
  $form['#entity'] = $entity;
  //$form['#params'] = $params;
  
  $fields = section_load_fields($section, TRUE);
  $categories = array();
  foreach ($fields as $field) {
    $categories[$field->category][] = $field->machine_name;
  }
  
  foreach ($categories as $category => $field_names) {
    if (!empty($category)) {
      $form[$category] = array(
        '#type' => 'fieldset',
        '#title' => $category,
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
      );
    }
    else {
      $form[$category] = array();
    }
    
    foreach ($field_names as $machine_name) {
      $field = $fields[$machine_name];
      $value = isset($entity->{$field->machine_name}) ? $entity->{$field->machine_name} : $field->default_value;
      
      if ($field->type == 'file' || $field->type == 'image') {
        $form['#attributes']['enctype'] = "multipart/form-data";
        $form[$category] += section_build_file_field($field, $value);
      }
      elseif ($field->type == 'date' || $field->type == 'timestamp') {
        $form[$category] += section_build_date_field($field, $value);
      }
      else {
        $form[$category] += section_build_generic_field($field, $params, $value);
      }
    }
  }

  $form['actions'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );

  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

function section_build_form_validate($form, &$form_state) {
  $section = $form['#section'];
  
  $fields = section_load_fields($section, TRUE);
  $placeholders = array();
  foreach ($fields as $field) {
    if (!empty($field->min_length)) {
      if (!empty($form_state['values'][$field->machine_name]) && strlen($form_state['values'][$field->machine_name]) < $field->min_length) {
        form_set_error($field->machine_name, t("Value must not be shorter than @value", array('@value' => $field->min_length)));
      }
    }
    if (!empty($field->max_length)) {
      if (!empty($form_state['values'][$field->machine_name]) && strlen($form_state['values'][$field->machine_name]) > $field->max_length) {
        form_set_error($field->machine_name, t("Value must not be longer than @value", array('@value' => $field->max_length)));
      }
    }
    
    if ($field->type == 'date') {
      if (module_exists('date_popup')) {
        $placeholders['@' . $field->machine_name] = strtotime($form_state['values'][$field->machine_name]);
      }
      else {
        $d = $form_state['values'][$field->machine_name];
        $placeholders['@' . $field->machine_name] = strtotime($d['year'] . '-' . $d['month'] . '-' . $d['day']);
      }
    }
    else {
      $placeholders['@' . $field->machine_name] = "'" . addslashes($form_state['values'][$field->machine_name]) . "'";
    }
  }
  
  if (!empty($section->validation)) {
    global $language;
    
    $lines = array_values(array_map('trim', explode("\n", strtr($section->validation, $placeholders))));
    foreach ($lines as $line) {
      $line = array_values(array_map('trim', explode('|', $line)));
      if (count($line) >= 2) {
        $code = 'return ' . array_shift($line) . ';';
        $message = array('' => '');
        foreach ($line as $item) {
          $item = array_map('trim', explode(':' , $item, 2));
          if (count($item) == 1) {
            $message[''] = t($item[0]);
          }
          else {
            $message[$item[0]] = $item[1];
          }
        }
        
        if (eval($code)) {
          form_set_error('', empty($message[$language->language]) ? $message[''] : $message[$language->language]);
        }
      }
    }
  }
}

function section_build_form_submit($form, &$form_state) {
  $section = $form['#section'];
  $entity = $form['#entity'];
  $old_files = array();
  
  $fields = section_load_fields($section, TRUE);
  foreach ($fields as $field) {
    if ($field->type == 'file' || $field->type == 'image') {
      $old_files[$field->machine_name] = $entity->{$field->machine_name};
      
      if (!empty($form_state['values'][$field->machine_name . '_remove'])) {
        $entity->{$field->machine_name} = 0;
      }
      
      $validators = array();
      if (!empty($field->file_extensions)) {
        $validators['file_validate_extensions'] = array(str_replace(',', ' ', $field->file_extensions));
      }
      if (!empty($field->file_max_size)) {
        $validators['file_validate_size'] = array(parse_size($field->file_max_size));
      }
      if ($field->type == 'image') {
        $validators['file_validate_is_image'] = array();
      }
      if (!empty($field->file_upload_path)) {
        $dest = file_create_path($field->file_upload_path);
        file_check_directory($dest, FILE_CREATE_DIRECTORY);
      }
      else {
        $dest = file_directory_path();
      }
      $file = file_save_upload($field->machine_name, $validators, $dest);
      if (!empty($file)) {
        $entity->{$field->machine_name} = $file->fid;
      }
    }
    else {
      if (!isset($form_state['values'][$field->machine_name])) {
        $form_state['values'][$field->machine_name] = '';
      }
      
      if ($field->type == 'date' || $field->type == 'timestamp') {
        if (module_exists('date_popup')) {
          $entity->{$field->machine_name} = $form_state['values'][$field->machine_name];
        }
        else {
          $d = $form_state['values'][$field->machine_name];
          $entity->{$field->machine_name} = $d['year'] . '-' . $d['month'] . '-' . $d['day'];
        }
        if ($field->type == 'timestamp') {
          $entity->{$field->machine_name} = strtotime($entity->{$field->machine_name});
        }
      }
      else {
        $entity->{$field->machine_name} = $form_state['values'][$field->machine_name];
      }
    }
  }
  
  if (section_entity_save($section, $entity)) {
    foreach ($fields as $field) {
      if ($field->type == 'file') {
        if (!empty($entity->{$field->machine_name}) && $entity->{$field->machine_name} != $old_files[$field->machine_name]) {
          $file = (object) array(
            'fid' => $entity->{$field->machine_name},
          );
          file_set_status($file, FILE_STATUS_PERMANENT);
        }
        
        if (!empty($old_files[$field->machine_name]) && $entity->{$field->machine_name} != $old_files[$field->machine_name]) {
          $file = (object) array(
            'fid' => $old_files[$field->machine_name],
          );
          file_set_status($file, FILE_STATUS_TEMPORARY);
        }
      }
    }
    $attr = array(
      '@section' => $section->machine_name,
      '@id' => empty($section->remote_key) ? $entity->id : $entity->{$section->remote_key},
    );
    drupal_set_message(t('The entity <em>@id</em> has been saved successfully', $attr));
    watchdog('section', 'Entity @id (@section) saved', $attr);
  }
  else {
    $attr = array(
      '@section' => $section->machine_name,
      '@id' => empty($section->remote_key) ? $entity->id : $entity->{$section->remote_key},
      '@data' => print_r($entity, TRUE),
    );
    drupal_set_message(t('Failed to save the entity <em>@id</em>', $attr), 'error');
    watchdog('section', 'Entity @id (@section) save failed. Data @data', $attr, WATCHDOG_ERROR);
  }
}

function section_build_file_field($field, $value) {
  if (!empty($value)) {
    $file = db_fetch_object(db_query("SELECT * FROM {files} WHERE fid = %d", $value));
  }
  
  if (!empty($file)) {
    if ($field->type == 'image') {
      if (module_exists('imagecache')) {
        $image = theme(
          'imagecache',
          $field->imagecache_preset,
          $file->filepath,
          $file->filename,
          $file->filename
        );
      }
      else {
        $image = theme(
          'image',
          $file->filepath,
          $file->filename,
          $file->filename
        );
      }
      $options = array(
        'html' => TRUE,
        'options' => array(
          'language' => $GLOBALS['language'],
        ),
      );
      $form[$field->machine_name . '_link'] = array(
        '#value' => l($image, $file->filepath, $options),
        '#weight' => $field->weight,
      );
    }
    else {
      $form[$field->machine_name . '_link'] = array(
        '#type' => 'item',
        '#value' => l($file->filename, $file->filepath, array('options' => array('language' => $GLOBALS['language']))),
        '#weight' => $field->weight,
      );
    }
    
    $form[$field->machine_name . '_remove'] = array(
      '#type' => 'checkbox',
      '#title' => t('Remove file'),
      '#weight' => $field->weight + 0.1,
    );
  }
  
  $form[$field->machine_name] = array(
    '#type' => 'file',
    '#title' => $field->title,
    '#description' => $field->description,
    '#weight' => $field->weight + 0.2,
    '#required' => (bool) $field->required,
  );
  
  return $form;
}

function section_build_date_field($field, $value) {
  if (module_exists('date_popup')) {
    $form[$field->machine_name] = array(
      '#type' => 'date_popup',
      '#title' => $field->title,
      '#description' => $field->description,
      '#date_format' => 'd.m.Y',
      '#weight' => $field->weight,
      '#required' => (bool) $field->required,
    );
  }
  else {
    $form[$field->machine_name] = array(
      '#type' => 'date',
      '#title' => $field->title,
      '#description' => $field->description,
      '#weight' => $field->weight,
      '#required' => (bool) $field->required,
    );
  }
  
  if (module_exists('date_popup')) {
    if (!empty($value)) {
      if (!is_numeric($value)) {
        $value = strtotime($value);
      }
      $form[$field->machine_name]['#default_value'] = date('Y-m-d', $value);
    }
  }
  else {
    if (!empty($value)) {
      if (!is_numeric($value)) {
        $value = strtotime($value);
      }
    }
    else {
      // Show current date instead of 1970-01-01
      $value = $_SERVER['REQUEST_TIME'];
    }
    $form[$field->machine_name]['#default_value'] = array(
      'day' => date('j', $value),
      'month' => date('n', $value),
      'year' => date('Y', $value),
    );
  }
  
  return $form;
}

function section_build_generic_field($field, $params, $value) {
  $form[$field->machine_name] = array(
    '#type' => $field->type,
    '#title' => $field->title,
    '#description' => $field->description,
    '#weight' => $field->weight,
    '#required' => (bool) $field->required,
  );
  
  if ($field->type == 'textfield') {
    if (!empty($field->field_size)) {
      $form[$field->machine_name]['#size'] = $field->field_size;
    }
    if (!empty($field->max_length)) {
      $form[$field->machine_name]['#maxlength'] = $field->max_length;
    }
  }
  
  if ($field->type == 'textarea') {
    if (!empty($field->field_size)) {
      $form[$field->machine_name]['#rows'] = $field->field_size;
    }
  }
  
  if (in_array($field->type, array('select', 'checkboxes', 'radios'))) {
    $placeholders = array(
      '@entityid' => empty($params['entity_id']) ? 0 : $params['entity_id'],
      '@recordid' => empty($params['record_id']) ? 0 : $params['record_id'],
    );
    $form[$field->machine_name]['#options'] = section_build_field_options(
      $field->source,
      $field->required,
      $placeholders
    );
  }
  
  $form[$field->machine_name]['#default_value'] = $value;
  
  return $form;
}
