<?php
/**
 * @file
 * Functions for importing data into entities. 
 *
 */

  /**
   * Locations List
   *
   * Assemble super-query for gathering all the
   * probable location names into a single list,
   * ordered by distrinctness.
   */
  function xmlimport_getloclist() {
    $xiqueries = array();
    foreach(xmlimport_getlistqueries() as $query_array) {
      $xiqueries = array_merge($xiqueries, $query_array);
    }
    
    $result = db_query(
      "(" 
      . implode(") UNION DISTINCT (", $xiqueries)
      . ") "
      . "ORDER BY word_count DESC, strlen DESC, isstate DESC, arraykey ASC;"
    );

    foreach($result as $record) {
      $listlines[strtolower($record->arraykey)] = array(
          'primary_name' => $record->primary_name,
          'country' => $record->country,
          'province' => $record->province,
      );
    }

    $GLOBALS['_xmlimport_locationlist'] = $listlines;
  }


  /**
   * Match keyed array  
   *
   * Find exact match to a key from a global array 
   * within the target text and return its value. 
   * Target checking, like array keys, are case-sensitive, 
   * so we convert all to lowercase on both sides to make
   * case-insensitive. 
   */
    function xmlimport_matchexactloc($target) {
      // Remove punctuation (comma, period, dash)
      $target = preg_replace("/[.,-]/", "", $target);
      
      // Remove dead words
      $target = str_ireplace('currently in ', '', $target);
      $target = str_ireplace('still in ', '', $target);
      
      if(!strlen($target)) return FALSE;
      
      // Bring all to lowercase to make case-insensitive.
      $target = strtolower($target);

      // Eliminate anything in parenthesis and brackets.
      $target = preg_replace("/\([^)]+\)/", "", $target);
      $target = preg_replace("/\[[^)]+\]/", "", $target);
        
      // Remove redundant spaces (2 or more together)
      $target = trim(preg_replace("/\s{2,}/", " ", $target));

      if(array_key_exists($target, $GLOBALS['_xmlimport_locationlist'])) {
        return $GLOBALS['_xmlimport_locationlist'][$target];
      } else {
        return FALSE;
      }
    }

  /**
   * Search keyed array  
   *
   * Find the first occurrence of a key from a global array 
   * within the target text and return its value. 
   */
    function xmlimport_searchloc($target) {







/*
      foreach($GLOBALS[$globalname] as $keyname => $value) {
if($keyname == "Arlington") print "Checking '$keyname' inside '$target'. RESULT: '" . stripos($target, $keyname) . "'.<br />\n";
        if(stripos($target, $keyname) !== FALSE) {
print "FOUND '$keyname' inside '$target'<br />\n";
          return $value;
        }
*/
      }


  /**
   * Populate Queries
   *
   * Assemble all possible queries into an array of strings.
   */
  function xmlimport_getlistqueries() {
    /* COUNTRY LISTS QUERY ----------------------- */
    $xiqueries['statesqueries'] = array(
      "states" => "
        /* States Vocab */
        SELECT 
          s.name as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(s.name) - LENGTH(REPLACE(s.name, ' ', ''))+1) as word_count, 
          LENGTH(s.name) as strlen, 
          1 as isstate
        FROM taxonomy_term_data s 
        INNER JOIN field_data_field_iso2 s2 ON s2.entity_id = s.tid 
        WHERE s.vid = 21
      ",
      
      "states_abbr" => "
        /* States Abbr */
        SELECT 
          s2.field_iso2_value as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(s2.field_iso2_value) - LENGTH(REPLACE(s2.field_iso2_value, ' ', ''))+1) as word_count, 
          LENGTH(s2.field_iso2_value) as strlen, 
          1 as isstate
        FROM taxonomy_term_data s 
        INNER JOIN field_data_field_iso2 s2 ON s2.entity_id = s.tid 
        WHERE s.vid = 21
      ",
      
      "states_syn" => "
        /* States Vocab Synonyms */
        SELECT 
          REPLACE(REPLACE(ss.synonyms_synonym_value,',',''),'.','') as arraykey, 
          s.name as primary_name, 
          'us' as country, 
          s2.field_iso2_value as province, 
          (LENGTH(ss.synonyms_synonym_value) - LENGTH(REPLACE(ss.synonyms_synonym_value, ' ', ''))+1) as word_count, 
          LENGTH(ss.synonyms_synonym_value) as strlen, 
          1 as isstate 
        FROM field_data_synonyms_synonym ss
        INNER JOIN taxonomy_term_data s ON s.tid = ss.entity_id AND s.vid = 21 
        INNER JOIN field_data_field_iso2 s2 ON s2.entity_id = s.tid
      ",
    );
      
    $xiqueries['countriesqueries'] = array(
      "countries" => "
        /* Countries Vocab */
        SELECT 
          REPLACE(REPLACE(REPLACE(c.name,',',''),'.',''),'-','') as arraykey, 
          c.name as primary_name, 
          c2.field_iso2_value as country, 
          NULL as province, 
          (LENGTH(c.name) - LENGTH(REPLACE(c.name, ' ', ''))+1) as word_count, 
          LENGTH(c.name) as strlen, 
          0 as isstate
        FROM taxonomy_term_data c 
        INNER JOIN field_data_field_iso2 c2 ON c2.entity_id = c.tid 
        WHERE c.vid = 16
      ",
      
      "countries_locations" => "
        /* Countries from Location Module, 252 non-political designations */
        SELECT 
          REPLACE(REPLACE(REPLACE(cc.name,',',''),'.',''),'-','') as arraykey, 
          cc.name as primary_name, 
          cc.iso2 as country, 
          NULL as province, 
          (LENGTH(cc.name) - LENGTH(REPLACE(cc.name, ' ', ''))+1) as word_count, 
          LENGTH(cc.name) as strlen, 
          0 as isstate 
        FROM countries_country cc 
      ",
      
      "countries_official" => "
        /* Countries Table official names */
        SELECT 
          REPLACE(REPLACE(cc.official_name,',',''),'.','') as arraykey, 
          cc.name as primary_name, 
          cc.iso2 as country, 
          NULL as province, 
          (LENGTH(cc.official_name) - LENGTH(REPLACE(cc.official_name, ' ', ''))+1) as word_count, 
          LENGTH(cc.official_name) as strlen, 
          0 as isstate 
        FROM countries_country cc 
      ",
      
      "countries_syn" => "
        /* Countries Vocab Synonyms */
        SELECT 
          REPLACE(REPLACE(REPLACE(cs.synonyms_synonym_value,',',''),'.',''),'-','') as arraykey, 
          c.name as primary_name, 
          c2.field_iso2_value as country, 
          NULL as province, 
          (LENGTH(cs.synonyms_synonym_value) - LENGTH(REPLACE(cs.synonyms_synonym_value, ' ', ''))+1) as word_count, 
          LENGTH(cs.synonyms_synonym_value) as strlen, 
          0 as isstate 
        FROM field_data_synonyms_synonym cs
        INNER JOIN taxonomy_term_data c ON c.tid = cs.entity_id AND c.vid = 16 
        INNER JOIN field_data_field_iso2 c2 ON c2.entity_id = c.tid
      ",
    );
 
     $xiqueries['citiesqueries'] = array(
      "cities_us" => "
        /* US Cities Vocab */
        SELECT 
          REPLACE(REPLACE(city.name,',',''),'.','') as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(city.name) - LENGTH(REPLACE(city.name, ' ', ''))+1) as word_count, 
          LENGTH(city.name) as strlen, 
          1 as isstate
        FROM taxonomy_term_data city 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 s2 ON s2.field_iso2_value = l.province AND s2.bundle = 'states' 
        INNER JOIN taxonomy_term_data s ON s.tid = s2.entity_id AND s.vid = 21
        WHERE city.vid = 22
        AND l.country = 'us'
      ",
      
      "cities_us_syn" => "
        /* US Cities Vocab Synonyms */
        SELECT 
          REPLACE(REPLACE(REPLACE(ss.synonyms_synonym_value,',',''),'.',''),'-','') as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(ss.synonyms_synonym_value) - LENGTH(REPLACE(ss.synonyms_synonym_value, ' ', ''))+1) as word_count, 
          LENGTH(ss.synonyms_synonym_value) as strlen, 
          1 as isstate
        FROM field_data_synonyms_synonym ss
        INNER JOIN taxonomy_term_data city ON city.tid = ss.entity_id AND city.vid = 22 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 s2 ON s2.field_iso2_value = l.province AND s2.bundle = 'states' 
        INNER JOIN taxonomy_term_data s ON s.tid = s2.entity_id AND s.vid = 21
        WHERE l.country = 'us'
      ",
      
      "cities_foreign" => "
      /* Foreign Cities Vocab */
        SELECT 
          REPLACE(REPLACE(city.name,',',''),'.','') as arraykey, 
          cc.name as primary_name, 
          l.country as country, 
          NULL as province, 
          (LENGTH(city.name) - LENGTH(REPLACE(city.name, ' ', ''))+1) as word_count, 
          LENGTH(city.name) as strlen, 
          0 as isstate
        FROM taxonomy_term_data city 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN countries_country cc ON cc.iso2 = l.country 
        WHERE city.vid = 22
        AND l.country != 'us'
      ",
      
      "cities_foreign_syn" => "
        /* Foreign Cities Vocab Synonyms */
        SELECT 
          REPLACE(REPLACE(REPLACE(ss.synonyms_synonym_value,',',''),'.',''),'-','') as arraykey, 
          cc.name as primary_name, 
          l.country as country, 
          NULL as province, 
          (LENGTH(city.name) - LENGTH(REPLACE(city.name, ' ', ''))+1) as word_count, 
          LENGTH(city.name) as strlen, 
          0 as isstate
        FROM field_data_synonyms_synonym ss
        INNER JOIN taxonomy_term_data city ON city.tid = ss.entity_id AND city.vid = 22 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN countries_country cc ON cc.iso2 = l.country 
        WHERE l.country != 'us'
      ",
    );
    
    $xiqueries['citycombosqueries'] = array(
      "citycombos_us" => "
        /* US Cities Vocab */
        SELECT 
          REPLACE(REPLACE(CONCAT(city.name, ' USA'),',',''),'.','') as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(CONCAT(city.name, ' USA')) - LENGTH(REPLACE(CONCAT(city.name, ' USA'), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(city.name, ' USA')) as strlen, 
          1 as isstate
        FROM taxonomy_term_data city 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 s2 ON s2.field_iso2_value = l.province AND s2.bundle = 'states' 
        INNER JOIN taxonomy_term_data s ON s.tid = s2.entity_id AND s.vid = 21
        WHERE city.vid = 22
        AND l.country = 'us'
      ",
      
      "citycombos_us_state" => "
        /* US Cities Vocab Combos */
        SELECT 
          REPLACE(REPLACE(CONCAT(city.name, ' ', s.name),',',''),'.','') as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(CONCAT(city.name, ', ', s.name)) - LENGTH(REPLACE(CONCAT(city.name, ', ', s.name), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(city.name, ', ', s.name)) as strlen, 
          1 as isstate
        FROM taxonomy_term_data city 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 s2 ON s2.field_iso2_value = l.province AND s2.bundle = 'states' 
        INNER JOIN taxonomy_term_data s ON s.tid = s2.entity_id AND s.vid = 21
        WHERE city.vid = 22
        AND l.country = 'us'
      ",
      
      "citycombos_us_state_syn" => "
        /* US Cities Vocab Combos */
        SELECT 
          REPLACE(REPLACE(CONCAT(city.name, ' ', ss.synonyms_synonym_value),',',''),'.','') as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(CONCAT(city.name, ', ', ss.synonyms_synonym_value)) - LENGTH(REPLACE(CONCAT(city.name, ', ', ss.synonyms_synonym_value), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(city.name, ', ', ss.synonyms_synonym_value)) as strlen, 
          1 as isstate
        FROM taxonomy_term_data city 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 s2 ON s2.field_iso2_value = l.province AND s2.bundle = 'states' 
        INNER JOIN taxonomy_term_data s ON s.tid = s2.entity_id AND s.vid = 21
        INNER JOIN field_data_synonyms_synonym ss ON ss.entity_id = s.tid 
        WHERE city.vid = 22
        AND l.country = 'us'
      ",
      
      "citycombos_us_city_syn_st" => "
        /* US Cities Vocab Combos */
        SELECT 
          REPLACE(REPLACE(CONCAT(scity.synonyms_synonym_value, ' ', s2.field_iso2_value),',',''),'.','') as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(CONCAT(scity.synonyms_synonym_value, ' ', s2.field_iso2_value)) - LENGTH(REPLACE(CONCAT(scity.synonyms_synonym_value, ' ', s2.field_iso2_value), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(scity.synonyms_synonym_value, ' ', s2.field_iso2_value)) as strlen, 
          1 as isstate
        FROM field_data_synonyms_synonym scity 
        INNER JOIN taxonomy_term_data city ON city.tid = scity.entity_id AND city.vid = 22 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 s2 ON s2.field_iso2_value = l.province AND s2.bundle = 'states' 
        INNER JOIN taxonomy_term_data s ON s.tid = s2.entity_id AND s.vid = 21
        WHERE l.country = 'us'
      ",
      
      "citycombos_us_state_usa" => "
        /* US Cities Vocab Combos */
        SELECT 
          REPLACE(REPLACE(CONCAT(city.name, ', ', s.name, ' usa'),',',''),'.','') as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(CONCAT(city.name, ', ', s.name, ', usa')) - LENGTH(REPLACE(CONCAT(city.name, ', ', s.name, ', usa'), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(city.name, ', ', s.name, ', usa')) as strlen, 
          1 as isstate
        FROM taxonomy_term_data city 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 s2 ON s2.field_iso2_value = l.province AND s2.bundle = 'states' 
        INNER JOIN taxonomy_term_data s ON s.tid = s2.entity_id AND s.vid = 21
        WHERE city.vid = 22
        AND l.country = 'us'
      ",
      
      "citycombos_us2" => "
        /* US Cities Vocab Combos */
        SELECT 
          REPLACE(REPLACE(CONCAT(city.name, ' ', s2.field_iso2_value),',',''),'.','') as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(CONCAT(city.name, ', ', s2.field_iso2_value)) - LENGTH(REPLACE(CONCAT(city.name, ', ', s2.field_iso2_value), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(city.name, ', ', s2.field_iso2_value)) as strlen, 
          1 as isstate
        FROM taxonomy_term_data city 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 s2 ON s2.field_iso2_value = l.province AND s2.bundle = 'states' 
        INNER JOIN taxonomy_term_data s ON s.tid = s2.entity_id AND s.vid = 21
        WHERE city.vid = 22
        AND l.country = 'us'
      ",
      
      "statecombos_us" => "
        /* US States Vocab Combos */
        SELECT 
          CONCAT(s.name, ', usa') as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(CONCAT(s.name, ', usa')) - LENGTH(REPLACE(CONCAT(s.name, ', usa'), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(s.name, ', usa')) as strlen, 
          1 as isstate
        FROM taxonomy_term_data s 
        INNER JOIN field_data_field_iso2 s2 ON s2.entity_id = s.tid 
        WHERE s.vid = 21
      ",
      
      "citycombos_us_syn" => "
        /* US Cities Vocab Combos Synonyms */
        SELECT 
          REPLACE(REPLACE(CONCAT(ss.synonyms_synonym_value, ', ', s2.field_iso2_value),',',''),'.','') as arraykey, 
          s.name as primary_name, 
          'us' as country,
          s2.field_iso2_value as province, 
          (LENGTH(CONCAT(ss.synonyms_synonym_value, ', ', s2.field_iso2_value)) - LENGTH(REPLACE(CONCAT(ss.synonyms_synonym_value, ', ', s2.field_iso2_value), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(ss.synonyms_synonym_value, ', ', s2.field_iso2_value)) as strlen, 
          1 as isstate
        FROM field_data_synonyms_synonym ss
        INNER JOIN taxonomy_term_data city ON city.tid = ss.entity_id AND city.vid = 22 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 s2 ON s2.field_iso2_value = l.province AND s2.bundle = 'states' 
        INNER JOIN taxonomy_term_data s ON s.tid = s2.entity_id AND s.vid = 21
        WHERE l.country = 'us'
      ",
      
      "citycombos_foreign" => "
        /* Foreign Cities Vocab Combos */
        SELECT 
          REPLACE(REPLACE(CONCAT(city.name, ', ', cc.name),',',''),'.','') as arraykey, 
          cc.name as primary_name, 
          l.country as country, 
          NULL as province, 
          (LENGTH(CONCAT(city.name, ', ', cc.name)) - LENGTH(REPLACE(CONCAT(city.name, ', ', cc.name), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(city.name, ', ', cc.name)) as strlen, 
          0 as isstate
        FROM taxonomy_term_data city 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN countries_country cc ON cc.iso2 = l.country 
        WHERE city.vid = 22
        AND l.country != 'us'
      ",
      
      "citycombos_foreign_syn" => "
        /* Foreign Cities Vocab Combos Synonyms */
        SELECT 
          REPLACE(REPLACE(CONCAT(ss.synonyms_synonym_value, ', ', cc.name),',',''),'.','') as arraykey, 
          cc.name as primary_name, 
          l.country as country, 
          NULL as province, 
          (LENGTH(CONCAT(ss.synonyms_synonym_value, ', ', cc.name)) - LENGTH(REPLACE(CONCAT(ss.synonyms_synonym_value, ', ', cc.name), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(ss.synonyms_synonym_value, ', ', cc.name)) as strlen, 
          0 as isstate
        FROM field_data_synonyms_synonym ss
        INNER JOIN taxonomy_term_data city ON city.tid = ss.entity_id AND city.vid = 22 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN countries_country cc ON cc.iso2 = l.country 
        WHERE l.country != 'us'
      ",

      "citycombos_foreign_country_syn" => "
        /* Foreign Cities Vocab Combos */
        SELECT 
          REPLACE(REPLACE(CONCAT(city.name, ', ', ss.synonyms_synonym_value),',',''),'.','') as arraykey, 
          c.name as primary_name, 
          l.country as country, 
          NULL as province, 
          (LENGTH(CONCAT(city.name, ', ', ss.synonyms_synonym_value)) - LENGTH(REPLACE(CONCAT(city.name, ', ', ss.synonyms_synonym_value), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(city.name, ', ', ss.synonyms_synonym_value)) as strlen, 
          0 as isstate
        FROM taxonomy_term_data city 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 c2 ON c2.field_iso2_value = l.country
        INNER JOIN field_data_synonyms_synonym ss ON ss.entity_id = c2.entity_id
        INNER JOIN taxonomy_term_data c ON c.tid = ss.entity_id AND c.vid = 16 
        WHERE city.vid = 22
        AND l.country != 'us'
      ",
      
      "citycombos_foreign_city_syn_country_syn" => "
        /* Foreign Cities Vocab Combos */
        SELECT 
          REPLACE(REPLACE(CONCAT(scity.synonyms_synonym_value, ', ', ss.synonyms_synonym_value),',',''),'.','') as arraykey, 
          c.name as primary_name, 
          l.country as country, 
          NULL as province, 
          (LENGTH(CONCAT(scity.synonyms_synonym_value, ', ', ss.synonyms_synonym_value)) - LENGTH(REPLACE(CONCAT(scity.synonyms_synonym_value, ', ', ss.synonyms_synonym_value), ' ', ''))+1) as word_count, 
          LENGTH(CONCAT(scity.synonyms_synonym_value, ', ', ss.synonyms_synonym_value)) as strlen, 
          0 as isstate
        FROM field_data_synonyms_synonym scity
        INNER JOIN taxonomy_term_data city ON city.tid = scity.entity_id AND city.vid = 22 
        INNER JOIN field_data_field_city_location cl ON cl.entity_id = city.tid 
        INNER JOIN location l ON l.lid = cl.field_city_location_lid 
        INNER JOIN field_data_field_iso2 c2 ON c2.field_iso2_value = l.country
        INNER JOIN field_data_synonyms_synonym ss ON ss.entity_id = c2.entity_id
        INNER JOIN taxonomy_term_data c ON c.tid = ss.entity_id AND c.vid = 16 
        WHERE city.vid = 22
        AND l.country != 'us'
      ",
    );
  
    return $xiqueries;
  }