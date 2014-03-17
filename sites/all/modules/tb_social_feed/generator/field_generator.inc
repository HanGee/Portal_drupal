<?php
function tb_social_feed_text_generate($field_value, $object, $field, $instance, $bundle) {
  $object_field = array();
  $object_field['value'] = $field_value;
  $object_field['format'] = 'full_html';
  return $object_field;
}

function tb_social_feed_image_generate($url, $object, $field, $instance, $bundle) {
  $info = pathinfo($url);
  $filename = isset($info['filename']) ? $info['filename'] : "";
  $extension = isset($info['extension']) ? $info['extension'] : 'png';

  $object_field = array();
  $path = tb_social_feed_download_image($url);
  if ($path) {
    $source = new stdClass();
    $source->uri = $path;
    $source->uid = 1;
    $source->filemime = 'image/' . pathinfo($path, PATHINFO_EXTENSION);
    $destination_dir = $field['settings']['uri_scheme'] . '://' . $instance['settings']['file_directory'];
    file_prepare_directory($destination_dir, FILE_CREATE_DIRECTORY);
    $counter = 0;
    $destination = $destination_dir . '/' . $filename . "." . $extension;
    $realpath = drupal_realpath($destination); 
    while (file_exists($realpath)) {
      $counter ++;
      $destination = $destination_dir . '/' . $filename . ($counter ? "_" . $counter : "") . "." . $extension;
      $realpath = drupal_realpath($destination); 
    }
    $source->filename = $filename . ($counter ? "_" . $counter : "") . "." . $extension;
    $file = file_move($source, $destination, FILE_CREATE_DIRECTORY);
  }
  else {
    return NULL;
  }

  $object_field['fid'] = $file->fid;
  $object_field['alt'] = '';
  $object_field['title'] = '';
  return $object_field;  
}

function tb_social_feed_download_image($url) {
  $info = pathinfo($url);
  $extension = isset($info['extension']) ? $info['extension'] : 'png';
  $tmp_file = drupal_tempnam('temporary://', 'imagefield_');
  if ($tmp_file) {
    try {   
            //Prevent the server is denying our request 
            ini_set('user_agent','Mozilla/4.0 (compatible; MSIE 6.0)'); 
            $content = file_get_contents($url);
    } catch (Exception $e) {            
    }
    $destination = $tmp_file . '.' . $extension;
    $path = file_unmanaged_move($tmp_file, $destination, FILE_CREATE_DIRECTORY);
    if ($path) {
        file_put_contents(drupal_realpath($path), $content);
    } else {
        return NULL;
    }
  }
  return $destination;
}

function tb_social_feed_taxonomy_generate($field_value, $object, $field, $instance, $bundle) {
  $field_value = trim($field_value);
  $tid = db_query('SELECT tid FROM {taxonomy_term_data} WHERE name = :name', array(':name' => $field_value))->fetchField();
  if ($tid === FALSE) {
    return NULL;
  }
  $object_field = array();
  $object_field['tid'] = (int) $tid;
  return $object_field;
}

function tb_social_feed_file_generate($field_value, $object, $field, $instance, $bundle) {
  return array("");
}

function tb_social_feed_number_generate($field_value, $object, $field, $instance, $bundle) {
  return array(is_numeric($field_value) ? $field_value : 0);
}

function tb_social_feed_date_generate($field_value, $object, $field, $instance, $bundle) {
  $time = strtotime($field_value);
  $str = date('Y-m-d H:i:s', $time); 
  return array('value' => $str);
}

function tb_social_feed_link_generate($field_value, $object, $field, $instance, $bundle){
  $object_field = array();
  $object_field['url'] = $field_value;
  $object_field['title'] = '';
  $object_field['attributes'] = array();
  return $object_field;
}


function tb_social_feed_youtube_generate($field_value, $object, $field, $instance, $bundle){
  $object_field = array();
  $object_field['input'] = $field_value;
  $data = explode('=', $field_value);
  $object_field['video_id'] = $data[count($data)-1];
  return $object_field;
}

