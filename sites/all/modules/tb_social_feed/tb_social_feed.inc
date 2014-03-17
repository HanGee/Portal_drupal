<?php

module_load_include('inc', 'tb_social_feed', 'generator/field_generator');
module_load_include('inc', 'tb_social_feed', 'generator/node_generator');

class TBSocialFeed {

    protected $configs;
    public static $USER_PAGE = 0;
    public static $FAN_PAGE = 1;
    
    public function setConfig($cfg) {
        $this->configs = $cfg;
    }

    public function getConfig() {
        return $this->configs;
    }

    public function getType() {
        return "All";
    }

    public function crawlPage($url) {
        if (function_exists('curl_init')) {
            $curl = curl_init();
            curl_setopt($curl, CURLOPT_URL, $url);
            curl_setopt($curl, CURLOPT_HEADER, 0);
            curl_setopt($curl, CURLOPT_TIMEOUT, 600);
            curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);
            curl_setopt($curl, CURLOPT_USERAGENT, 'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.7.5) Gecko/20041107 Firefox/1.0');
            $content = curl_exec($curl);
            curl_close($curl);
        } else {
            $content = @file_get_contents($url);
        }
        return $content;
    }

    public function getContent() {
        return NULL;
    }

    public function parseData($content) {
        return NULL;
    }

    public function feed() {
        $db_contents = new Tb_Social_Feed_Contents();
        set_time_limit(3600);
        $results = $this->getContent();
        $nodes = array();
        if (!is_array($results))
            return $nodes;        
        foreach ($results as $row) {            
            $existed = $db_contents->select(array(), array(
                'feed_id'   =>  $row['feed_id'],
                'account'   =>  $row['account'],
                'service'   =>  $row['service']
            ));   
            if ($existed) {
                if ($row['changed'] > $existed->changed) {
                    $node = tb_social_feed_generate_node($row, $existed->node_id);
                    $row_contents = array(
                        'feed_id' => $row['feed_id'],
                        'data' => serialize($row),
                        'service' => $row['service'],
                        'account' => $row['account'],
                        'created' => $row['created'],
                        'changed' => $row['changed'],
                        'url' => $row['url'],
                        'reference_url' => $row['reference_url'],
                        'latest_feed' => time(),
                        'node_id' => $node->nid,
                    );
                    db_update('tb_social_feed_contents')
                            ->fields($row_contents)
                            ->condition('id', $existed->id)
                            ->execute();
                    $nodes[] = $node;
                }
            } else {
//                $row['field_tb_wall_badge'] = 'tb-wall-new-badge';//TODO -Removed 09-09-2013
                $node = tb_social_feed_generate_node($row);
                $row_contents = array(
                    'feed_id' => $row['feed_id'],
                    'data' => serialize($row),
                    'service' => $row['service'],
                    'account' => $row['account'],
                    'created' => $row['created'],
                    'changed' => $row['changed'],
                    'url' => $row['url'],
                    'reference_url' => $row['reference_url'],
                    'latest_feed' => time(),
                    'node_id' => $node->nid,
                );
                db_insert('tb_social_feed_contents')->fields($row_contents)->execute();
                $nodes[] = $node;
            }
        }
        return $nodes;
    }

    function createServiceForm($service, &$form) {
        $db_accounts = new Tb_Social_Feed_Accounts();
        $form[$service->service . "_enable"] = array(
            '#type' => 'checkbox',
            '#title' => t("Enable"),
            '#value' => $service->enabled,
        );

        $settings_form = array(
            '#type' => 'container',
            '#states' => array(
                'visible' => array(
                    "input[name='{$service->service}_enable']" => array(
                        'checked' => TRUE,
                    ),
                ),
            ),
        );

        $this->createAppForm($service, $settings_form);

        $settings_form[$service->service . "_auto_feed"] = array(
            '#type' => 'checkbox',
            '#title' => t("Auto feed"),
            '#value' => $service->auto_feed,
            '#description' => 'Automatically feed items every configured time of the drupal cron. Click <a href="' . base_path() . '/admin/config/system/cron">here</a> to configure cron ',
        );
        $this->createFeedLimitForm($service, $settings_form);
        $accounts = $db_accounts->select(array(), array(
                        'service' => $service->service,
                        'deleted' => 0
                    ), array(
                        'weight' => 'ASC'
                    ), Tb_Social_Feed_Accounts::$RETURN_ARRAY);
        
        $accounts_form = array(
            '#type' => 'fieldset',
            '#collapsible' => TRUE,            
            '#title' => t("Profile")
        );
        $n = count($accounts) ? count($accounts) : 1;
        $sub_form = array();
        for ($i = 0; $i < $n; $i++) {
            $account = isset($accounts[$i]) ? $accounts[$i] : new stdClass();
            $account->form_order = $i;
            $account->service = isset($account->service) ? $account->service : $service->service;
            $account->account_name = isset($account->account_name) ? $account->account_name : "";
            $this->createAccountForm($account, $sub_form);
            if ($sub_form) {
                $accounts_form[$service->service . "_account_wrapper_$i"] = $sub_form;
            }
        }

        if ($sub_form) {
            $settings_form[$service->service . "_accounts_wrapper"] = $accounts_form;
            $settings_form[$service->service . "_accounts_info"] = array('#type' => 'hidden', '#value' => "");
            $settings_form[$service->service . "_add_account_wrapper"] = array(
                '#type' => 'container',
                '#attributes' => array('class' => array('form-wrapper', 'tb-feed-add-account-wrapper')),
            );
            $settings_form[$service->service . "_add_account_wrapper"]['add-account-button'] = array(
                '#markup' => '<div class="add-account-button-wrapper"><a href="javascript:void(0)" class="btn btn-add-account" onclick="Drupal.TBFeed.addAccount(\'' . $service->service . '\')">Add Account</a></div>',
            );
            $account = new stdClass();
            $account->form_order = 0;
            $account->service = $service->service;
            $account->account_name = "";
            $this->createAccountForm($account, $sub_form);
            $settings_form[$service->service . "_add_account_wrapper"][$service->service . "_account_wrapper"] = $sub_form;
        }
        
        //Admin profile
        //REMOVE FOR FIXING BUG ON CANVAS URL
//        $admin_profile = array();
//        global $user;
//        $current_account = $db_accounts->select(array(), array(
//            'uid'   => $user->uid,
//            'service'   =>  $service->service
//        ));
//        $this->createAdminProfileForm($service, $admin_profile, $current_account);
//        $settings_form[$service->service . 'admin_profile'] = $admin_profile;
        
        //Fan pages
        $fan_pages = array();              
        $this->createFanPageForm($service, $fan_pages); 
        if (!empty($fan_pages)) {
            //Facebook
            $settings_form[$service->service . 'fan_pages'] = $fan_pages;   
        }        
        $form[$service->service . '_settings_wrapper'] = $settings_form;        
    }

    function createFeedLimitForm($service, &$form) {
        $form[$service->service . "_feed_limit"] = array(
            '#type' => 'textfield',
            '#title' => t("Feed Limit"),
            '#value' => $service->feed_limit,
            '#description' => t('Please enter a valid number, the number of feed items returned for each feed request.Enter 0 if you want to get the maximum feed items.'),
        );
    }

    function createAppForm($service, &$form) {
        
    }
    /**
     * 
     * @param type $service
     * @param type $input
     * @param type $service_fields
     * @date 09-09-2013
     */
    function extendedInfo($service, &$input, &$service_fields) {
    }
    function createAccountForm($account, &$form) {        
    }
    
    function createAdminProfileForm($params, &$form, $user = NULL) {
    }
    function createFanPageForm($params, &$form) {
        $form = NULL;
    }

    public function rssToArray($tags, $array, $url) {

        $doc = new DOMdocument();
        @$doc->load($url);
        $rss_array = array();
        foreach ($tags as $tag) {
            if ($doc->getElementsByTagName($tag)) {
                foreach ($doc->getElementsByTagName($tag) AS $node) {
                    $items = array();
                    foreach ($array AS $key => $values) {
                        $items[$key] = array();
                        foreach ($values as $value) {
                            if ($itemsCheck = $node->getElementsByTagName($value)) {
                                for ($j = 0; $j < $itemsCheck->length; $j++) {
                                    if (($attribute = $itemsCheck->item($j)->nodeValue) != "") {
                                        $items[$key][] = $attribute;
                                    } elseif ($attribute = $itemsCheck->item($j)->getAttribute('term')) {
                                        $items[$key][] = $attribute;
                                    } elseif ($itemsCheck->item($j)->getAttribute('rel') == 'alternate') {
                                        $items[$key][] = $itemsCheck->item($j)->getAttribute('href');
                                    }
                                }
                            }
                        }
                    }
                    array_push($rss_array, $items);
                }
            }
        }
        return $rss_array;
    }

    /**
     * Return feed limit based on number of feeds & user-defined feed limit
     * @param $items array list of items
     * @return $limit number 
     */
    public function getFeedLimit($items) {
        $length = count($items);
        $limit = $this->configs['limit'];
        if ($limit == 0 || $limit > $length)
            $limit = $length;
        return $limit;
    }

    function getFeedType() {
        return "SocialFeed";
    }

    public function getBundle() {
        return 'tb_social_feed';
    }
    public function getFeedTypes(){
        return array();
    }

}
