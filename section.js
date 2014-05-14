Drupal.behaviors.section = function(context) {
  $('#edit-type').change(function () {
    switch($(this).val()) {
      case 'textfield':
      case 'textarea':
        $('#edit-field-size-wrapper').show();
        $('#edit-min-length-wrapper').show();
        $('#edit-max-length-wrapper').show();
        $('#edit-file-extensions-wrapper').hide();
        $('#edit-file-max-size-wrapper').hide();
        $('#edit-file-upload-path-wrapper').hide();
        $('#edit-imagecache-preset-wrapper').hide();
        $('#edit-source-wrapper').hide();
        break;
        
      case 'date':
      case 'timestamp':
      case 'checkbox':
      case 'hidden':
        $('#edit-field-size-wrapper').hide();
        $('#edit-min-length-wrapper').hide();
        $('#edit-max-length-wrapper').hide();
        $('#edit-file-extensions-wrapper').hide();
        $('#edit-file-max-size-wrapper').hide();
        $('#edit-file-upload-path-wrapper').hide();
        $('#edit-imagecache-preset-wrapper').hide();
        $('#edit-source-wrapper').hide();
        break;
        
      case 'select':
      case 'checkboxes':
      case 'radios':
        $('#edit-field-size-wrapper').hide();
        $('#edit-min-length-wrapper').hide();
        $('#edit-max-length-wrapper').hide();
        $('#edit-file-extensions-wrapper').hide();
        $('#edit-file-max-size-wrapper').hide();
        $('#edit-file-upload-path-wrapper').hide();
        $('#edit-imagecache-preset-wrapper').hide();
        $('#edit-source-wrapper').show();
        break;
        
      case 'image':
        $('#edit-imagecache-preset-wrapper').show();
      case 'file':
        $('#edit-field-size-wrapper').hide();
        $('#edit-min-length-wrapper').hide();
        $('#edit-max-length-wrapper').hide();
        $('#edit-file-extensions-wrapper').show();
        $('#edit-file-max-size-wrapper').show();
        $('#edit-file-upload-path-wrapper').show();
        $('#edit-source-wrapper').hide();
        break;
        
      default:
        $('#edit-field-size-wrapper').show();
        $('#edit-min-length-wrapper').show();
        $('#edit-max-length-wrapper').show();
        $('#edit-file-extensions-wrapper').show();
        $('#edit-file-max-size-wrapper').show();
        $('#edit-file-upload-path-wrapper').show();
        $('#edit-imagecache-preset-wrapper').show();
        $('#edit-source-wrapper').show();
        break;
    }
  }).trigger('change');
  
  $('#edit-sql-type-type').change(function() {
    var val = $(this).val();
    
    if (val == 'numeric') {
      $('#edit-sql-type-precission-wrapper').show();
      $('#edit-sql-type-scale-wrapper').show();
    }
    else {
      $('#edit-sql-type-precission-wrapper').hide();
      $('#edit-sql-type-scale-wrapper').hide();
    }
    
    if (val == 'varchar' || val == 'char') {
      $('#edit-sql-type-length-wrapper').show();
    }
    else {
      $('#edit-sql-type-length-wrapper').hide();
    }
    
    if (val == 'int' || val == 'serial') {
      $('#edit-sql-type-unsigned-wrapper').show();
    }
    else {
      $('#edit-sql-type-unsigned-wrapper').hide();
    }
    
    if (val == 'text' || val == 'blob') {
      $('#edit-sql-type-not-null-wrapper').hide();
      $('#edit-sql-type-default-wrapper').hide();
    }
    else {
      $('#edit-sql-type-not-null-wrapper').show();
      $('#edit-sql-type-default-wrapper').show();
    }
  }).trigger('change');
};