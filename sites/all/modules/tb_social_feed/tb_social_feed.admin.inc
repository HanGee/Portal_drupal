<?php
function tb_social_feed_admin_settings($form) {
    
    $form['tb_social_feed'] = array(
        '#type' => 'vertical_tabs',
        '#weight' => - 10,
        '#default_tab' => variable_get('tb_social_feed__active_tab'),
    );
    $services = tb_social_feed_get_services();
    $array_names = array();
    foreach ($services as $service) {
        module_load_include('inc', 'tb_social_feed', $service->inc);
        $class_name = $service->class;
        $feed_object = new $class_name();
        $service_form = array(
            '#type' => 'fieldset',
            '#title' => $service->title,
            '#collapsible' => TRUE,
            '#collapsed' => !$service->enabled,
        );
        $array_names[] = $service->service;
        $feed_object->createServiceForm($service, $service_form);
        $form['tb_social_feed'][$service->service . "_wrapper"] = $service_form;   
    }

    $form['tb_social_feed']['tb_social_feed_services'] = array('#type' => 'hidden', '#value' => implode(" ", $array_names));
    $form['actions'] = array('#type' => 'actions');
    $form['actions']['tb_social_feed_manually'] = array(
        '#type' => 'hidden',
        '#value' => 0,
    );
    $form['actions']['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Save')
    );
    $form['actions']['feed_now'] = array(
        '#markup' => '<input type="submit" value="' . t('Save & Feed') . '" class="form-submit" onclick="return Drupal.TBFeed.saveAndFeed();">',
    );
    return $form;
}

function tb_social_feed_admin_settings_validate($form, $form_state) {
  return FALSE;
}

/**
 * 
 * @global type $user
 * @param type $form
 * @param type $form_state
 * @date 09-09-2013
 */
function tb_social_feed_admin_settings_submit($form, $form_state) {
    global $user;
    $db_accounts = new Tb_Social_Feed_Accounts();
    $db_services = new Tb_Social_Feed_Services();
    
    $input = $form_state['input']; 
    
    variable_set('tb_social_feed__active_tab', $input['tb_social_feed__active_tab']);
    $feed_services = tb_social_feed_get_services();
    foreach ($feed_services as $key => $service) {
        $service_fields = array(
            'enabled' => isset($input[$service->service . '_enable']) && $input[$service->service . '_enable'] ? 1 : 0,
            'auto_feed' => isset($input[$service->service . '_auto_feed']) && $input[$service->service . '_auto_feed'] ? 1 : 0,
            'feed_limit' => isset($input[$service->service . '_feed_limit']) ? intval($input[$service->service . '_feed_limit']) : 0,
            'app_info' => isset($input[$service->service . '_app_info']) ? serialize($input[$service->service . '_app_info']) : serialize(array()),
            'changed' => time()            
        );
        //TODO: move into another function        
        $feed_object = new $service->class;
        $feed_object->extendedInfo($service, $input, $service_fields);      
        $number_accounts = 0;
        if (!empty($input[$service->service . '_enable'])) {                    
            if (isset($input[$service->service . '_feed_account']) && is_array($input[$service->service . '_feed_account'])) {
                $accounts = $input[$service->service . '_feed_account'];
                $current_accounts = $input[$service->service . '_feed_current_account'];
                $account_name = $input[$service->service . '_feed_account_name'];

                $db_accounts->update(array('deleted' => 1), array('service' => $service->service));
                
                foreach ($current_accounts as $weight => $current_account) { 
                    $current_account = trim($current_account);
                    $account = trim($accounts[$weight]);
                    if (!empty($current_account) || !empty($account)) {
                        $acc = $db_accounts->select(array(), array(
                            'account' => $current_account,
                            'service' => $service->service
                        ));
                        $number_accounts++;
                        if ($acc) {
                            $fields = array(
                                'weight' => $weight,
                                'account_name' => $account_name[$weight],
                                'changed' => time(),
                                'deleted' => 0,
                                'feed_limit' => intval($input[$service->service . "_feed_limit"]),
                                'account' => $account,
                                'feed_type' => isset($service_fields['feed_type'])?intval($service_fields['feed_type']):NULL
                            );
                            $db_accounts->update($fields, array(
                                'account' => $current_account,
                                'service' => $service->service
                            ));                            
                        } else {
                            $fields = array(
                                'account' => $account,
                                'account_name' => $account_name[$weight],
                                'service' => $service->service,
                                'weight' => $weight,
                                'deleted' => 0,
                                'enabled' => 1,
                                'auto_feed' => isset($input[$service->service . "_auto_feed"])?1:0,
                                'feed_limit' => intval($input[$service->service . "_feed_limit"]),
                                'latest_feed' => 0,
                                'created' => time(),
                                'changed' => time(),
                                'access_token'  =>  isset($input[$service->service ."_admin_access_token"])?$input[$service->service ."_admin_access_token"]:"",
                                'feed_type' => isset($service_fields['feed_type'])?intval($service_fields['feed_type']):NULL,
                                'uid'   =>  $user->uid,
                                'type'  => isset($input[$service->service ."_type"][$weight])?$input[$service->service ."_type"][$weight]:0
                            );
                            $db_accounts->insert($fields);                            
                        }
                    }
                }
            } else {
                $number_accounts = -1;
            }
        }
        $service_fields['number_accounts'] = $number_accounts;
        $db_services->update($service_fields, array(
            'service' => $service->service
        ));
        
    }
    
    if ($input['tb_social_feed_manually']) {
        tb_social_feed_manual();  
        $result = db_select('tb_social_feed_contents', 'c')
                ->fields('c')
                ->execute()
                ->fetchAssoc();
        drupal_set_message(t('Saved & Feeded'));
    } else {
        drupal_set_message(t('Saved'));
    }
}


function tb_social_feed_manual() {
    $feed_services = tb_social_feed_get_services();    
    foreach ($feed_services as $key => $service) {
        if ($service->enabled) {
            $configs = array(
                'feed_limit'    =>  $service->feed_limit,
                'app_info'      =>  unserialize($service->app_info),
                'service'       =>  $service->service
            );
            
            $class_name = $service->class;
            if ($service->enabled && class_exists($class_name)) {
                if ($service->number_accounts >= 0) {
                    $db_accounts = new Tb_Social_Feed_Accounts();

                    $accounts = $db_accounts->select(array(), array(
                                    'service' => $service->service,
                                    'deleted' => 0
                                ), array(), Tb_Social_Feed_Accounts::$RETURN_ARRAY);
                    foreach ($accounts as $account) {
                        $configs['account'] = $account->account;
                        $configs['feed_type'] = $account->feed_type;
                        $configs['access_token'] = $account->access_token;
                        $configs['type']    =   $account->type;
                        $feed = new $class_name($configs);
                        $results = $feed->feed();
                    }
                } else {
                    $feed = new $class_name($configs);
                    $results = $feed->feed();
                }
            }
        }
    }
}