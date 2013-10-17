<?php
/**
 * @file
 * Functions for importing data into entities. 
 *
 */
 
  /**
   * Extracts the entity data from its database and table wrappers.
   * 
   * This function assumes a standard phpMyAdmin sort uf dump, 
   * where the outer wrapper is the database name, 
   * the second wrapper is the primary table name, 
   * and that there is only ONE of each. 
   *
   * RETURNS: array $data containing array $data['rows'] of entity records. 
   */
    function xmlimport_unwrap($xml_array) {
      $data = array();
      $keys = array_keys($xml_array);
        foreach($xml_array as $data['dbname'] => $table) {
          foreach($table as $data['tablename'] => $data['rows']) {
            return($data);
          }
        }
    }

  /**
   * Steps through the array of entity data. 
   *
   * $guid_field_name must match both the name of the field in
   * the bundle AND the index name of the field in your data array.
   */
    function xmlimport_step($rows, $offset_iterations, $max_iterations) {
      module_load_include('inc', 'xmlimport', $name = 'xmlimport.locations');
      
      // SET CONSTANTS
      xmlimport_hashtables();
      xmlimport_getloclist();
      smlimport_titlearray();
      $iteration_count = 0;
      $savecount = 0;
      $failcount = 0;
      $skipcount = 0;

      foreach($rows as $index => $row) {
        // Control the iterations.
        if($index < $offset_iterations) continue;
        if($max_iterations > 0 && $savecount >= $max_iterations) break;
        
        // Review each row.
        $iteration_count++;
        if(!empty($row['entity_id'])) {
          $row['guid'] = "entity_id " . $row['entity_id'];
        }
        print "<p><strong>Record $index: type " . $row['entity_type'] . ", GUID " . $row['guid'] . ".</strong></p>\n";    
        xmlimport_fieldproc($row);
//$entity1 = entity_load('node',array(18124));
//print "entity1: "; print_r($entity1); exit;
        if($entity = xmlimport_match($row)) {
          // Save entity. Don't test $result for FALSE boolean, as that only indicates saving without specific instruction.
          xmlimport_stripnullvalues($entity);
//print_r($entity);//exit;

          // Activate revisioning, if set in content type.
          if($row['entity_type'] == 'node') {
            $entity->revision = in_array('revision',variable_get('node_options_' . $entity->type, array('status', 'promote')));
          }

          entity_save($row['entity_type'], $entity);
//print_r($entity); exit;
          //Test for success.
          if($row['entity_type'] == "comment") {
            if($entity->cid) { 
              $savecount++;
              print "<p>-->Successfully saved to <a href=\"/" . $row['entity_type'] . "/$entity->cid\" target=\"_blank\">cid '$entity->cid'</a>.</p>\n";
            } else {
              $failcount++;
              print "<p>-->FAILED to save " . $row['entity_type'] . " entity (no ID).</p>\n";
            }
          } elseif($row['entity_type'] == "user") {
            if($entity->uid) { 
              $savecount++;
              print "<p>-->Successfully saved to <a href=\"/user/$entity->uid\" target=\"_blank\">uid '$entity->uid'</a>.</p>\n";
            } else {
              $failcount++;
              print "<p>-->FAILED to save " . $row['entity_type'] . " entity (no ID).</p>\n";
            }
          } elseif($row['entity_type'] == "taxonomy_term") {
            if($entity->tid) { 
              $savecount++;
              print "<p>-->Successfully saved to <a href=\"/taxonomy/term/$entity->tid\" target=\"_blank\">tid '$entity->uid'</a>.</p>\n";
            } else {
              $failcount++;
              print "<p>-->FAILED to save " . $row['entity_type'] . " entity (no ID).</p>\n";
            }
          } else {
            if($entity->nid) { 
              $savecount++;
              print "<p>-->Successfully saved to <a href=\"/node/$entity->nid\" target=\"_blank\">nid '$entity->nid'</a>.</p>\n";
            } else {
              $failcount++;
              print "<p>-->FAILED to save " . $row['entity_type'] . " entity (no ID).</p>\n";
            }
          }
        } else {
          $skipcount++;
        }
      }
      
      print "<hr />\n";
      print "<h2>Summary</h2>\n";
      print "<p><strong>" . ($index + 1) . " rows traversed</strong></p>\n";
      print "<p><strong>$iteration_count rows reviewed</strong></p>\n";
      print "<p><strong>$savecount entities successfully saved</strong></p>\n";
      print "<p><strong>$skipcount existing entities skipped</strong></p>\n";
      print "<p><strong>$failcount entities failed to save</strong></p>\n";
    }

  /**
   * Looks for a matching entity in the database and loads it. 
   * If no match is found, then instantiate a new entity. 
   *
   * $guid_field_name must match both the name of the field in
   * the bundle AND the index name of the field in your data array.
   * 
   * Returns FALSE if the entity should not be saved. 
   */
    function xmlimport_match($row) {
      unset($entity);
      if(empty($row['type'])) $row['type'] = $row['bundle'];
      if(!empty($row['entity_id'])) { 
        $entity = array_shift(entity_load($row['entity_type'], array($row['entity_id']))); 
      } else {
        $query = new EntityFieldQuery();
        $result = $query
          ->entityCondition('entity_type', $row['entity_type'])
          ->entityCondition('bundle', $row['bundle'])
          ->fieldCondition($row['guid_field'], $row['guid_column'], $row['guid'], '=')
          ->execute();
      }
      if(isset($entity) || isset($result[$row['entity_type']])) {
        // Entity already exists.
        if(variable_get('xmlimport_saveexisting')) {
          // Load existing entity.
          if(!isset($entity)) {
            $entity = entity_load_single($row['entity_type'], array_shift(array_keys($result[$row['entity_type']])));
          }
          // Replace properties with the new ones found in $row.
          $admin_fields = array('entity_type', 'entity_type_attr', 'bundle', 'bundle_attr', 'type', 'guid_field', 'guid_column', 'guid', 'index', 'body_attr', 'entity_id', 'entity_id_attr');
          $CHANGED = FALSE;
          foreach($row as $fieldname => $property){
            if(!in_array($fieldname, $admin_fields)) { // Review only data fields, not administrative fields.
             // Change only if different.
              if($fieldname != 'changed' && xmlimport_changed($entity->$fieldname, $property)) {
                $CHANGED = TRUE;
                xmlimport_mergeproperties($entity->$fieldname, $property);
              }
            }
          }
          if($CHANGED) {
            print "<p>-->Updated existing entity of type '" . $row['entity_type'] . "'</p>\n";
            return $entity;
          } else {
            print "<p>-->Nothing to change in existing entity of type '" . $row['entity_type'] . "'</p>\n";
            return FALSE;
          }
        } else {
          return FALSE;
        }
      } else {
        // Entity does not exist, add new entity.
        if(variable_get('xmlimport_savenew')) {
          print "<p>-->Create entity from scratch of type '" . $row['entity_type'] . "'</p>\n";
          // Create new entity. Bundle name must be stored in key 'type' for entity_create().
          return entity_create($row['entity_type'], $row);
        } else {
         return FALSE;
        }
      }
    }
    
  /**
   * Compare current and candidate values 
   *
   * Returns boolean to indicate if the values of the given
   * property are equivalent. Ignores collateral values, such as safe_value and format.
   * Returns TRUE if values are DIFFERENT or not equivalent.  
   */
    function xmlimport_changed($current_property, $candidate_property) {
      if($current_property == $candidate_property) {
        return FALSE;
      } elseif(is_array($current_property) !== is_array($candidate_property)) {
        return TRUE;
      } elseif(is_array($candidate_property)) {
        foreach($candidate_property['und'] as $index => $valuearray) {
          foreach($valuearray as $key => $value) {
            if(empty($current_property['und'][$index][$key])) {
              if(!empty($value)) return TRUE;
            } elseif($current_property['und'][$index][$key] != $value) {
              return TRUE;
            }
          }
        }
        return FALSE;
      } else {
        return TRUE;
      }
    }

  /* FIELD PROCESSORS ---------------------------------------------------------------- */
  
  /**
   * Run processors on fields 
   *
   * Main processor for assembling the properties for 
   * each field.  
   */
    function xmlimport_fieldproc(&$row) {
      // Match author.
      if(!empty($row['field_auxauthor'])) {
        xmlimport_fieldproc_authorreg($row);
      }

      // Find image_record node for photo reference.
      if(!empty($row['field_field_aux_imageref'])) {
        xmlimport_fieldproc_photoreg($row);
      }

      // Find parent node for comments.
      if($row['entity_type'] == 'comment') {
        xmlimport_fieldproc_commentreg($row);
        xmlimport_fieldproc_getcommentername($row);
        xmlimport_fieldproc_getcommenterlocation($row);
      }
      
      // Process ALL Fields
      foreach($row as $fieldname => $value) {
        // Compensate for values set as empty arrays by XML parser.
        if($value == array()) $value = NULL;
        switch ($fieldname) {
        // Don't treat base properties unless they need filtering. 
          case 'entity_type':
            break;
          case 'entity_type_attr':
            break;
          case 'entity_id':
            break;
          case 'entity_id_attr':
            break;
          case 'bundle':
            break;
          case 'type':
            break;
          case 'vid':
            break;
          case 'guid_field':
            break;
          case 'guid_column':
            break;
          case 'guid':
            break;
          case 'index':
            break;
          case 'title':
            $row[$fieldname] = xmlimport_fieldproc_html2literal($value);
            break;
          case 'body':
            if($row['summary'] == array()) $row['summary'] = NULL;
            $row[$fieldname] = xmlimport_fieldenclose(
              array(
                array(
                  'value' => xmlimport_fieldproc_htmlentities($value),
                  'summary' => xmlimport_fieldproc_htmlentities($row['summary']),
                  'format' => 'filtered_html',
                )
              )
            );
            unset($row['summary']);
            break;
          case 'summary':
            $row[$fieldname] = xmlimport_fieldenclose(xmlimport_fieldproc_htmlentities($value));
            break;
          case 'subject':
            $row[$fieldname] = xmlimport_fieldproc_htmlentities($value);
            break;
          case 'status':
            break;
          case 'pid':
            break;
          case 'uid':
            break;
          case 'language':
            break;
          case 'node_type':
            break;
          case 'in_new':
            break;
          case 'cid':
            break;
          case 'tid':
            break;
          case 'nid':
            break;
          case 'hostname':
            break;
          case 'created':
            break;
          case 'changed':
            break;
          case 'thread':
            break;
          case 'name':
            break;
          case 'pass':
            require_once 'includes/password.inc';
            $row[$fieldname] = user_hash_password($value);
            break;
          case 'mail':
            break;
          case 'homepage':
            break;
          case 'uuid':
            break;
          case 'registered_name': //blank for anonymous users.
            break;
          case 'u_uid':
            break;
          case 'signature':
            break;
          case 'signature_format':
            break;
          case 'picture':
            break;
          case 'new':
            break;
          case 'comment_body':
            $row[$fieldname] = xmlimport_fieldenclose(
              array(
                array(
                  'value' => xmlimport_fieldproc_htmlentities($value),
                  'format' => 'filtered_html',
                )
              )
            );
            break;
          case 'field_description':
            $row[$fieldname] = xmlimport_fieldenclose(
              xmlimport_fieldproc_htmlentities($value)
            );
            break;
          case 'field_aux_bio':
            $row[$fieldname] = xmlimport_fieldenclose(
              xmlimport_fieldproc_htmlentities($value)
            );
            break;
          case 'field_user_account':
            $row[$fieldname] = xmlimport_fieldenclose(array('target_id' => $value));
            break;
          case 'field_email':
            $row[$fieldname] = xmlimport_fieldenclose(array('email' => $value));
            break;
          case 'field_tags':
            $row[$fieldname] = 
             (!empty($value) 
              ? xmlimport_fieldenclose(xmlimport_fieldproc_gettidsbyarray(explode(',',$value),'tags')) 
              : $value
              );
            break;            
          case 'field_region':
            $row[$fieldname] = 
              (!empty($value) 
              ? xmlimport_fieldenclose(xmlimport_fieldproc_gettidsbyarray(explode(',',$value),'regions')) 
              : $value
              );
            break;            
          case 'field_categories':
            $row[$fieldname] = 
              (!empty($value) 
              ? xmlimport_fieldenclose(xmlimport_fieldproc_gettidsbyarray(explode(',',$value),'category')) 
              //? xmlimport_fieldenclose(_taxonomy_get_tid_from_term(array_shift(taxonomy_get_term_by_name($value,'category'))),'tid') 
              : $value
              );
            break;            
          case 'field_country':
            $row[$fieldname] = 
              (!empty($value) 
              ? xmlimport_fieldenclose(xmlimport_fieldproc_gettidsbyarray(explode(',',$value),'countries')) 
              : $value
              );
            break;            
          case 'field_topic':
            $row[$fieldname] = 
              (!empty($value) 
              ? xmlimport_fieldenclose(xmlimport_fieldproc_gettidsbyarray(explode(',',$value),'topics')) 
              : $value
              );
            break;            
          case 'field_external_embed_video':
            $row[$fieldname] = 
              (!empty($value) 
              ? xmlimport_fieldenclose(
                  array(
                    array(
                      'video_url' => xmlimport_fieldproc_urlfromhtmobject($value),
                      //'thumbnail_url' => $row['field_aux_thumbnail_link'],
                    )
                  )
                )
              : $value
              );
            break;            
          case 'field_brightcove_video':
            $row[$fieldname] = 
              (!empty($value) 
              ? xmlimport_fieldenclose(
                  array(
                    array(
                      'video_id' => xmlimport_fieldproc_urlfromhtmobject($value),
                    )
                  )
                )
              : $value
              );
            break;            
          default: 
            $row[$fieldname] = xmlimport_fieldenclose($value);
        }
      }
             
      // Validate 'changed' date.
      if(!empty($row['changed']) && !empty($row['created']))
        xmlimport_fieldproc_validatechanged($row['changed'], $row['created']);
    }
    

    /**
     * Parses video HTML objects for the URL 
     *
     * Look for HTML <object> or <iframe> and extract the URL.
     */
      function xmlimport_fieldproc_urlfromhtmobject($value) {
        $DOM = new DOMDocument;
        $DOM->loadHTML($value);
        $embed = $DOM->getElementsByTagName('embed');
        // Embedded object video
        if($embed->length) {
          return $embed->item(0)->getAttribute('src');
  
        // iFramed video
        } else {
          $iframe = $DOM->getElementsByTagName('iframe');
          if($iframe->length) {
            return $iframe->item(0)->getAttribute('src');
          } else {
            return $value;
          }
        }
      }
      
      
    /**
     * Data field processor  
     *
     * Locate the author based on the username in auxauthor.
     */
      function xmlimport_fieldproc_authorreg(&$row) {
        if($user = user_load_by_name($row['field_auxauthor'])) {
          print "-->Matched name with auxauthor '" . $row['field_auxauthor'] . "' with user '" . $user->uid . "'<br />\n";
          $row['uid'] = $user->uid;
        } else {
          print "-->Author node NOT FOUND for field_auxauthor '" . $row['field_auxauthor'] . "'.<br />\n";
        }
      }    
  
    /**
     * Data field processor  
     *
     * Locate the image_record based on matching the auxid.
     */
      function xmlimport_fieldproc_photoreg(&$row) {
        $query = new EntityFieldQuery();
        $result = $query
          ->entityCondition('entity_type', 'node')
          ->entityCondition('bundle', 'image_record')
          ->fieldCondition('field_auxid', 'value', $row['field_field_aux_imageref'], '=')
          ->execute();
  
        if(isset($result['node'])) {
          print "-->Matched photo with auxid '" . $row['field_field_aux_imageref'] . "' with <a href=\"/node/" . array_shift(array_keys($result['node'])) . "\" target=\"_blank\">referenced photo '" . array_shift(array_keys($result['node'])) . "'</a><br />\n";
          $row['field_photo'] = array(array('target_id' => key($result['node'])));        
        } else {
          print "<p>-->Referenced photo NOT FOUND for auxid '" . $row['field_field_aux_imageref'] . "'.</p>\n";
        }
      }    
  
    /**
     * Data field processor  
     *
     * Locate a comment's parent nid based on matching the auxid.
     */
      function xmlimport_fieldproc_commentreg(&$row) {
        list($entity_type, $parent_entity_type, $parent_bundle) = explode('_',$row['bundle']);
        $query = new EntityFieldQuery();
        $result = $query
          ->entityCondition('entity_type', $parent_entity_type)
          ->entityCondition('bundle', $parent_bundle)
          ->fieldCondition('field_auxid', 'value', $row['field_auxid'], '=')
          ->execute();
  
        if(isset($result[$parent_entity_type])) {
          print "-->Matched comment with auxid '" . $row['field_auxid'] . "' with <a href=\"/node/" . array_shift(array_keys($result[$parent_entity_type])) . "\" target=\"_blank\">parent node '" . array_shift(array_keys($result[$parent_entity_type])) . "'</a><br />\n";
          $row['nid'] = array_shift(array_keys($result[$parent_entity_type]));        
        } else {
          print "<p>-->Parent node NOT FOUND for auxid '" . $row['field_auxid'] . "'.</p>\n";
        }
      }    
  
    /**
     * Get the commenter's name value  
     *
     * Populate the commenter's first and last names 
     * from the aux field. 
     */
      function xmlimport_fieldproc_getcommentername(&$row) {
        $auxname = $row['field_aux_commenter_name'];

        // First eliminate anything in parenthesis and brackets.
        $auxname = preg_replace("/\([^)]+\)/", "", $auxname);
        $auxname = preg_replace("/\[[^)]+\]/", "", $auxname);
        
        // Remove redundant spaces (2 or more together)
        $auxname = preg_replace("/\s{2,}/", " ", $auxname);
        
        // Translate special characters.
        $auxname = xmlimport_fieldproc_literalentities($auxname);
        
        // Remove suffixes (everything from the first comma on)
        $auxname = preg_replace("/,.*$/", "", $auxname);        

        // Remove possessives (everything from the apostrophe-s on)
        $auxname = preg_replace("/'s .*$/", "", $auxname);        
        
        $auxname = explode(" ", trim($auxname));
        if(count($auxname)) {
          $name['given'] = array_shift($auxname);
          // Remove titles and prefixes. 
          if(in_array($name['given'],$GLOBALS['_xmlimport_nametitles']) && count($auxname)) {
            $name['given'] = array_shift($auxname);
          }
        
          if(count($auxname)) {
            $name['family'] = array_pop($auxname);
          }
        }
        
        // Historical exceptions
        /*
        if(strcasecmp($name['given'],"palgye") == 0) {
          $name['given'] = "Hyun-Joo";
        }     
        */
        
        $row['field_commenter_name'] = array($name);
      }
      
    /**
     * Get the commenter's location value  
     *
     * Populate the commenter's location from the aux field. 
     * Use the body field to supplement. 
     */
      function xmlimport_fieldproc_getcommenterlocation(&$row) {
        // Render blanks correctly
        if($row['field_aux_commenter_location'] === array()) $row['field_aux_commenter_location'] = "";
        
        // Prevent misaligned data dumps. 
        if(strlen($row['field_aux_commenter_credit']) > 80) $row['field_aux_commenter_credit'] = NULL;

        // Eliminate submitted URL's
        if(stripos($row['field_aux_commenter_location'], "http") !== FALSE || stripos($row['field_aux_commenter_location'], "www.") !== FALSE) return FALSE;

        if(!$loc = xmlimport_matchexactloc($row['field_aux_commenter_location'])) {
          if(!$loc = xmlimport_matchexactloc(xmlimport_locationfromcommentcredit($row['field_aux_commenter_credit']))) {
            //if(!$loc = xmlimport_searchloc($row['field_aux_commenter_location'])) {
              //if(!$loc = xmlimport_searchloc(xmlimport_locationfromcommentcredit($row['field_aux_commenter_credit']))) {
                return FALSE;
              //}
            //}
          }
        }
        
        $commenter_location['name'] = $loc['primary_name']; // Necessary for a modicum of data for the new lid.
        $commenter_location['country'] = strtolower($loc['country']);
        $commenter_location['province'] = strtolower($loc['province']);
        
        $row['field_commenter_location'] = array($commenter_location);
      }


    /**
     * Parse auxiliary comment credit line for location.  
     *
     */
      function xmlimport_locationfromcommentcredit(&$field_aux_commenter_credit) {  
        $words = explode(" ", $field_aux_commenter_credit);
        array_pop($words);
        $FOUND_PREPOSITION = FALSE;
        $country = array();
        foreach($words as $word) {
          if($FOUND_PREPOSITION) $country[] = $word;
          if($word == "in" || $word == "from") {
            $FOUND_PREPOSITION = TRUE;
          }
        }
        if(count($country)) {
          return implode(" ", $country);
        } else {
          return FALSE;
        }
      }
      
    /**
     * Data field processor  
     *
     * Translate faulty html entities using a hash table.
     */
      function xmlimport_fieldproc_htmlentities($value) {      
        // Run filter
        foreach($GLOBALS['_xmlimport_htmlchars'] as $filter) {
          if(stripos($value, $filter['target']) !== FALSE) {
            $value = str_replace($filter['target'],$filter['value'],$value);
          }
        }
        
        return ($value);
      }
  
    /**
     * Data field processor  
     *
     * Translate faulty entities using a hash table.
     */
      function xmlimport_fieldproc_literalentities($value) {      
        // Run filter
        foreach($GLOBALS['_xmlimport_literalchars'] as $filter) {
          if(stripos($value, $filter['target']) !== FALSE) {
            $value = str_replace($filter['target'],$filter['value'],$value);
          }
        }
        
        return ($value);
      }
  
    /**
     * Data field processor  
     *
     * Translate strings with HTML entities to literal.
     */
      function xmlimport_fieldproc_html2literal($value) {
        return html_entity_decode($value, ENT_QUOTES, 'UTF-8'); 
      }
  
    /**
     * Data field processor  
     *
     * Ensure the changed date is not less than the created date.
     * In EE the changed date is more reasonable and earlier. 
     */
      function xmlimport_fieldproc_validatechanged($value, &$created) {
        if($created > $value) {
          $created = $value; 
        } else {
          return FALSE; 
        }
      }
  
    /**
     * Data field processor  
     *
     * Run taxonomy_get_term_by_name() on an array,
     * but return only an array of tid's.
     */
      function xmlimport_fieldproc_gettidsbyarray($values, $vocabulary = 'tags') {
        if(!is_array($values)) {
          $values = array($values);
        }
        
        foreach($values as $value) {
          $tids[] = array('tid' => key(taxonomy_get_term_by_name(trim($value), $vocabulary)));
        }
        
        return($tids);
      }
     
    /**
     * Data field processor  
     *
     * Adds a value key to each element of an indexed array.
     */
      function xmlimport_fieldproc_setidskeys($values, $valuekey) {        
        foreach($values as $value) {
          $ids[] = array($valuekey => trim($value));
        }
        
        return($ids);
      }
     
    /**
     * Data field processor  
     *
     * Encloses a field value within a property array. 
     */
      function xmlimport_fieldenclose($values) {
        if(!is_array($values)) {
          $values = array(array('value' => $values));
        }

        if(array_values($values) === $values) {
        // Array is indexed
          foreach($values as $value) {
            if($value == array()) $value = NULL;
            $property['und'][] = $value;
          }
        } else {
        // Array is associative
          foreach($values as $key => $value) {
            if($value == array()) {
              $values[$key] = NULL;
            } elseif($key == 'country' || $key == 'province') {
              // Location module requires lowercase country and state codes.
              $values[$key] = strtolower($value);
            }
            
          }
          $property['und'][] = $values;
        }

        return $property;
      }    
      
    /**
     * Merge entity properties  
     *
     * Overwrites like elements, while preserving values that 
     * don't exist in the replacement set.
     */
      function xmlimport_mergeproperties(&$property, $newproperty) {
        // Traverse the new property's values to insert into the old.
        if(is_array($newproperty)) {
          foreach($newproperty as $lang => $langarray) {
            foreach($langarray as $index => $valuearray) {
              foreach($valuearray as $valuekey => $value) {
                $property[$lang][$index][$valuekey] = $value;
              }
            }
          }
        } else {
          $property = $newproperty;
        }
      }

    /**
     * Remove NULL values  
     *
     * Reduce the property array to it's minimum by stripping
     * all elements with null values. 
     * Needed to prevent errors when saving or rendering. 
     */
      function xmlimport_stripnullvalues(&$entity) {
        foreach($entity as $fieldname => $property) {
          if(is_array($property)) {
            $fieldvalues = array();
            if(isset($property['und'])) {
              foreach($property['und'] as $valuearray) {
                $valuekeys = array();
                foreach($valuearray as $key => $value) {
                  if(isset($value)) {
                    $valuekeys[$key] = $value;
                  }
                }
                if(count($valuekeys)) $fieldvalues[] = $valuekeys;
              }
              $entity->$fieldname = (count($fieldvalues)) ? array('und' => $fieldvalues) : array();
            }
          }
        }
      }


  /**
   * Build table of name titles.
   *
   */
  function smlimport_titlearray(){
    define('NAMETITLES', "Mr.,Mrs.,Ms.,Miss,Dr.,Sir,Senator,Congressman,Judge,Chairman,Sec.,Secretary,Pres.,President,Capt.,Captain,Maj.,Major,Cmdr.,Commander,Col.,Colonel,LtCol.,Gen.,Gnl.,General,Mr,Mrs,Ms,Dr,Sec,Pres,Capt,Maj,Cmdr,Col,LtCol,Gen,Gnl,Father,Mother,Brother,Sister,Uncle,Aunt,Cousin,Grandpa,Grandma,Grandfather,Grandmother");
    $GLOBALS['_xmlimport_nametitles'] = explode(",", NAMETITLES);
  }
    
  
  /**
   * Build hash table for filtering bad string data.
   *
   */
  function xmlimport_hashtables() {
    $GLOBALS['_xmlimport_htmlchars'] = array(
      array('target' => html_entity_decode('&amp;#8226;&amp;#128;&quot;'), 'value' => '&#8212;'),
      array('target' => html_entity_decode('&amp;#8226;&amp;#128;(tm)'), 'value' => '\''),
      array('target' => html_entity_decode('&amp;#8226;&amp;#38;'), 'value' => '!'),
      array('target' => html_entity_decode('&amp;#8226;&amp;#29;'), 'value' => NULL),
      array('target' => html_entity_decode('&amp;#8226;&amp;#28;'), 'value' => '&#8220;'),
      array('target' => html_entity_decode('&amp;#8194;&amp;#160;'), 'value' => 'space'),
      array('target' => html_entity_decode('&amp;#8194;&amp;#161;'), 'value' => '&#161;'),
      array('target' => html_entity_decode('(tm)'), 'value' => '\''),
      array('target' => html_entity_decode('&amp;#128;'), 'value' => NULL),
      array('target' => html_entity_decode('&amp;#59450;'), 'value' => '&#8212'),
      array('target' => html_entity_decode('&amp;#180;'), 'value' => '\''),
      array('target' => html_entity_decode('&amp;#173;'), 'value' => NULL),
      array('target' => html_entity_decode('&lt;i&gt;'), 'value' => '<em>'),
      array('target' => html_entity_decode('&lt;/i&gt;'), 'value' => '</em>'),
      array('target' => html_entity_decode('&lt;b&gt;'), 'value' => '<strong>'),
      array('target' => html_entity_decode('&lt;/b&gt;'), 'value' => '</strong>'),
      array('target' => html_entity_decode('&amp;#8232;'), 'value' => NULL),
      array('target' => html_entity_decode('&amp;#8234;'), 'value' => NULL),
      array('target' => html_entity_decode('&amp;#65533;'), 'value' => NULL),
      array('target' => html_entity_decode('&amp;#700;s'), 'value' => '\''),
      array('target' => html_entity_decode('&amp;#8208;'), 'value' => '-'),
      array('target' => html_entity_decode('&amp;#128;'), 'value' => NULL),
      array('target' => html_entity_decode('&amp;#8236;'), 'value' => NULL),
      array('target' => html_entity_decode('&amp;#184;'), 'value' => ','),
      array('target' => 'â€™', 'value' => '&#8217;'),
      array('target' => 'ï¿½', 'value' => '&atilde;'),
    );

    $GLOBALS['_xmlimport_literalchars'] = array(
      array('target' => html_entity_decode("(™)"), 'value' => "\'"),
      array('target' => "â€™", 'value' => "’"),
      array('target' => "ï¿½", 'value' => "ã"),
    );
  }
      

  /**
   * Loads XML file into array.
   *
   */
  function xmlimport_xml2array($file, $get_attributes = 1, $priority = 'tag') {
    //Initialize.
    $contents = "";
    
    // Require XML Parser Functions available for PHP.
    if(!function_exists('xml_parser_create')) {
        return array ();
    }
    
    // Initialize parser object.
    $parser = xml_parser_create('');
    
    // Require input file.
    if(!($fp = @fopen($file, 'rb'))) {
        return array ();
    }
    while (!feof($fp)) {
        $contents .= fread($fp, 8192);
    }
    fclose($fp);
    
    // Configure parser object
    xml_parser_set_option($parser, XML_OPTION_TARGET_ENCODING, "UTF-8");
    xml_parser_set_option($parser, XML_OPTION_CASE_FOLDING, 0);
    xml_parser_set_option($parser, XML_OPTION_SKIP_WHITE, 1);
    xml_parse_into_struct($parser, trim($contents), $xml_values); // Data transfers to $xml_values.
    xml_parser_free($parser); // Destruct(?) the parser. 
    
    // Require data to have passed to $xml_values.
    if(!$xml_values) return; 
    
    // Initialize working arrays.    
    $xml_array = array ();
    $parents = array ();
    $opened_tags = array ();
    $arr = array();
    $current = &$xml_array;
    $repeated_tag_index = array ();
    
    // Step through data. 
    /* $xml_values is an indexed array of structured arrays:
     *   [tag] = xml tag name
     *   [type] = Whether it is an 'open' or 'close' tag or a one-part 'complete' tag.
     *   [level] = Hierarchical level within the structure.
     *   [value] = Data enclosed within a 'complete' tag.
     *   [attributes] = tag properties inside an 'open' tag that often have values, such as 'name="bob"'.
     */ 
    foreach ($xml_values as $data) {
      // Refresh working variables.
      unset ($attributes, $value);
      
      // Restructure "field" tags under name of the field (2013-07-12).
      if($data['tag'] == 'field' && !empty($data['attributes']['name'])) {
        $data['tag'] = $data['attributes']['name'];
        $data['attributes']['type'] = 'field';
      }

      // Load key/value pairs into variables (i.e. $tag, $value, $type, $attributes, $priority, etc.).
      extract($data);

      // Initialize arrays.
      $result = array ();
      $attributes_data = array ();
      
      // Load top level value.
      // $priority = 'tag' causes result to return value directly, rather than in array. 
      if(isset ($value)) {
        if($priority == 'tag')
          $result = $value;
        else
          $result['value'] = $value;
      }

      // Load attributes into a separate array. 
      if(isset ($attributes) and $get_attributes) {
        foreach ($attributes as $attr => $val) {
          if($priority == 'tag')
            $attributes_data[$attr] = $val;
          else
            $result['attr'][$attr] = $val; //Set all the attributes in a array called 'attr'
        }
      }
      
      // OPEN Tags
      if($type == "open") { 
        $parent[$level -1] = &$current;
        if(!is_array($current) or (!in_array($tag, array_keys($current)))) {
          $current[$tag] = $result;
          if($attributes_data)
            $current[$tag . '_attr'] = $attributes_data;
          $repeated_tag_index[$tag . '_' . $level] = 1;
          $current = & $current[$tag];
        }
        else {
          if(isset ($current[$tag][0])) {
            $current[$tag][$repeated_tag_index[$tag . '_' . $level]] = $result;
            $repeated_tag_index[$tag . '_' . $level]++;
          }
          else { 
            $current[$tag] = array (
              $current[$tag],
              $result
            ); 
            $repeated_tag_index[$tag . '_' . $level] = 2;
            if(isset ($current[$tag . '_attr'])) {
              $current[$tag]['0_attr'] = $current[$tag . '_attr'];
              unset ($current[$tag . '_attr']);
            }
          }
          $last_item_index = $repeated_tag_index[$tag . '_' . $level] - 1;
          $current = & $current[$tag][$last_item_index];
        }
      }
      
      // COMPLETE Tags
      elseif($type == "complete") {
        if(!isset ($current[$tag])) {
          $current[$tag] = $result;
          $repeated_tag_index[$tag . '_' . $level] = 1;
          if($priority == 'tag' and $attributes_data)
            $current[$tag . '_attr'] = $attributes_data;
        }
        else {
          if(isset ($current[$tag][0]) and is_array($current[$tag])) {
            $current[$tag][$repeated_tag_index[$tag . '_' . $level]] = $result;
            if($priority == 'tag' and $get_attributes and $attributes_data) {
              $current[$tag][$repeated_tag_index[$tag . '_' . $level] . '_attr'] = $attributes_data;
            }
            $repeated_tag_index[$tag . '_' . $level]++;
          }
          else {
            $current[$tag] = array (
              $current[$tag],
              $result
            ); 
            $repeated_tag_index[$tag . '_' . $level] = 1;
            if($priority == 'tag' and $get_attributes) {
              if(isset ($current[$tag . '_attr'])) { 
                $current[$tag]['0_attr'] = $current[$tag . '_attr'];
                unset ($current[$tag . '_attr']);
              }
              if($attributes_data) {
                $current[$tag][$repeated_tag_index[$tag . '_' . $level] . '_attr'] = $attributes_data;
              }
            }
            $repeated_tag_index[$tag . '_' . $level]++; //0 and 1 index is already taken
          }
        }
      }
      elseif($type == 'close') {
        $current = &$parent[$level -1];
      }
    }
    return ($xml_array);
  }
