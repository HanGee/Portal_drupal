<?php
module_load_include('php', 'tb_social_feed', 'libraries/facebook/facebook');
class TBFacebookFeed extends TBSocialFeed {

    private $_facebook;
    function __construct($configs = NULL) {
        $this->configs = $configs;                
        //$this->fbInitFacebook();
    }
    public static $HAS_IMAGE = 1;
   
    function getType() {
        return "Facebook";        
    }

    public function fbGetContentsFromUserPage() {        
        $account = $this->configs['account'];
//        $type = $this->getConnections($this->configs['feed_type']);
        $type = 'feed';
        $entries = $this->_facebook->api("/$account/$type/", array(
            'limit' =>  $this->configs['feed_limit']            
        ));      
        $contents = array();
        foreach ($entries['data'] as $entry) {
            if ($entry['type'] == 'photo') {
                $info =  array(
                    'feed_id'   =>  $entry['id'],
                    'account'   =>  $this->_facebook->getUser(),
                    'service'   =>  'facebook',
                    'title'     =>  $this->fbGetUsername(),
                    'body'  =>  $entry['message'],
                    'created'   => 1375624838,
                    'changed'   =>  1375624838,
                    'feed_type' =>  $this->getFeedType(),
                    'field_social_feed_type'    => $this->getType(),
                    'node_type' =>  $this->getBundle(),
                    'url'   =>  '',
                    'reference_url' =>  $entry['link'],
                    'field_reference_url' => $entry['link'] 
                );
                if (isset($entry['picture'])) {
                    $info['field_image'] = preg_replace('/_s\.jpg/', '_n.jpg', $entry['picture']);
                }            
                $contents[] = $info;
            }
        }      
        return $contents;
    }
    
    public function fbGetContentsFromFanPage(){
       //NOTES: Only get 20 feeds per request
        $url = "http://www.facebook.com/feeds/page.php?id={$this->configs['account']}&format=json&t=" . time();
        $content = $this->crawlPage($url);        
        $data = json_decode($content);
        if (!isset($data->entries)) {
            return FALSE;
        }
        $results = array();
        $data = $data->entries;
        $limit = (count($data) > $this->configs['feed_limit']) ? $this->configs['feed_limit'] : count($data);
        for ($i = 0; $i < $limit; $i++) {
            $row = $data[$i];
            $result = array(
                'feed_id' => $row->id,
                'account' => $this->configs['account'],
                'service' => 'facebook',
                'title' => $row->author->name
            );

            // get facebook image
            $content = $this->updateFacebookImage($row->content);
            $pattern = '/<img[^>]+src\s*=\s*"([^"]+)"/i';
            preg_match_all($pattern, $content, $matches);
            if (isset($matches[1][0])) {
                $result['field_image'] = $matches[1][0];
                // remove the image from content
                $content = preg_replace('/<img[^>]+src\s*=\s*"([^"]+)" +alt="\s*" *\/>/i', '', $content);
            }
            // fix url issues in the facebook feed content
            $pattern = '/href="\/l.php\?u=(http[^\'"]+)"/i';
            preg_match_all($pattern, $content, $matches);
            if (isset($matches[1][0])) {
                $url = $matches[1][0];
                $url_array = explode('&', $url);
                $url = 'href="' . rawurldecode($url_array[0]) . '"';
                $content = str_replace($matches[0][0], $url, $content);
            }

            $result['body'] = $content;
            $result['created'] = strtotime($row->published);
            $result['changed'] = strtotime($row->updated);
            $result['feed_type'] = $this->getFeedType();
            $result['field_social_feed_type'] = $this->getType();
            $result['node_type'] = $this->getBundle();
            $result['url'] = $url;
            $result['field_reference_url'] = $result['reference_url'] = $row->alternate;
            $results[] = $result;
        }
        return $results;
    }
    
    public function getContent() {
        $contents = array();
        if ($this->configs['type'] == self::$USER_PAGE) {
            $account = isset($this->configs['account'])?$this->configs['account']:'me';
            if ($this->fbIsValidAccessToken($account)) {
                $contents = $this->fbGetContentsFromUserPage();        
            }        
        } else if ($this->configs['type'] == self::$FAN_PAGE) {
            $contents = $this->fbGetContentsFromFanPage();
        }
        return $contents;
  }

  function createAppForm($service, &$form) {
//    $app_info = unserialize($service->app_info);
//    $form[$service->service . "_app_info[app_id]"] = array(
//        '#type' => 'textfield',
//        '#title' => t("App ID"),
//        '#value' => isset($app_info['app_id']) ? $app_info['app_id'] : "",
//    );
//    $form[$service->service . "_app_info[app_secret]"] = array(
//        '#type' => 'textfield',
//        '#title' => t("App Secret"),
//        '#value' => isset($app_info['app_secret']) ? $app_info['app_secret'] : "",
//    );
  }

  /**
   * 	Replace facebook safe image(small size) with original images( big size ) in it's content
   *
   * 	@param $post string Content of facebook item
   */
  private function updateFacebookImage($post) {
    $pattern = '/http[^\'\"\>\<]+(?:safe_image|app_full_proxy).php\?([^\"\'\>]+)/i';
    preg_match($pattern, $post, $matches);
    if (isset($matches[1])) {
      $params = str_replace('&amp;', '&', $matches[1]);

      $patternUrl = '/\&(?:url|src)=([^\"\'\&]+)/';
      preg_match($patternUrl, $params, $matches2);
      if (isset($matches2[1])) {
        $url = rawurldecode($matches2[1]);
        $post = str_replace($matches[0], $url, $post);
      }
    }
    else {
      $pattern = '/<img[^>]+src\s*=\s*"([^"]+)_s.([^"]+)"/i';
      preg_match($pattern, $post, $rb1);
      if (isset($rb1[1])) {
        $post = str_replace('_s.', '_n.', $post);
      }
    }
    return $post;
  }

  function createAccountForm($account, &$form) {
      return true;
    $form = array(
      '#type' => 'fieldset',
      '#title' => t("Account"),
      '#collapsible' => TRUE,
      '#collapsed'  =>  FALSE
        
    );
    $form[$account->service . '_feed_account_name[]'] = array(
      '#type' => 'textfield',
      '#size' => 8,
      '#title' => t("Account Name"),
      '#default_value' => isset($account->account_name) ? $account->account_name : ""
    );
    $form[$account->service . '_feed_account[]'] = array(
      '#type' => 'textfield',
      '#size' => 8,
      '#title' => t("Facebook ID"),
      '#default_value' => isset($account->account) ? $account->account : "",
      '#description' => t("Click <a href='http://findmyfacebookid.com/' target='_blank'>here</a> to find your Facebook ID"),
      '#attributes' => array('class' => array('account')),
    );
    $form[$account->service . '_feed_current_account[]'] = array(
      '#type' => 'hidden',
      '#value' => isset($account->account) ? $account->account : ""
    );
    $form[$account->service . '_feed_account_verify'] = array(
      '#markup' => '<a href="javascript:void(0)" class="form-submit btn-verify-account" onclick="Drupal.TBFeed.verifyAccount(\'http://www.facebook.com/feeds/page.php?format=rss20&id=\', this)">' . t('Check feed') . '</a><br/>',
    );
    $form[$account->service . '_feed_account_delete'] = array(
      '#markup' => '<a href="javascript:void(0)" class="btn btn-delete-account" onclick="Drupal.TBFeed.deleteAccount(this)">' . t('Delete') . '</a>',
    );
  }
  
  function createAdminProfileForm($params, &$form, $user = NULL) {
        //Setting feeds
        $form = array(
            '#type' => 'fieldset',
            '#title' => t("Your profile"),
            '#description'  =>  '<span class="tb-social-feed-description">Click <div class="tb-fb-login">here</div> to load your Facebook profile</span>',
            '#collapsible' => TRUE            
        );
        $form[$params->service . '_admin_profile'] = array(
            '#type' => 'container',
            '#attributes'   =>  array(
                'class' =>  array('facebook-admin-profile')
            )
        );
        $form[$params->service . '_admin_profile'][$params->service . '_admin_feed_type'] = array(
            '#type' => 'radios',
            '#title' => t('Entry type'),            
            '#options' => $this->getFeedTypes(),            
            '#default_value' => isset($user->feed_type) ? $user->feed_type : NULL            
        );
    }
    function createFanPageForm($params, &$form) {
        $db_accounts = new Tb_Social_Feed_Accounts();
        $pages = $db_accounts->select(array(), array(
                    'service' => $params->service,
                    'deleted'    => 0,
                    'type'  => self::$FAN_PAGE
                ), array(), Tb_Social_Feed_Accounts::$RETURN_ARRAY);        
         //Setting feeds
        $form = array(
            '#type' => 'fieldset',
            '#title' => t("Fan pages"),
            '#collapsible' => TRUE,
            '#description' => '<span class="tb-social-feed-description">Example: <a href="http://www.facebook.com/themebrain" target="_blank">http://www.facebook.com/themebrain</a></span>'
        );               
        $fan_page = array(
            '#type' => 'textfield',
            '#size' => 8,
            '#title' => t("Page profile"),
            '#default_value' =>"",
            '#description' => '<a href="javascript:void(0)" class="btn btn-delete-account" onclick="Drupal.TBFeed.deleteAccount(this)">' . t('Delete') . '</a>',
            '#attributes' => array('name' => array('facebook_fan_pages[]'))
        );
        if (!empty($pages)) {
            $index = 0;
            foreach ($pages as $page) {
                $graph = $this->fbGraph($page->account);
                $username = $graph->username;
                $fan_page['#default_value'] = "http://www.facebook.com/$username";
                $form[$params->service . "_fan_pages_$index"] = $fan_page;
                $index++;
            }            
        } else {
            $form[$params->service . '_fan_pages[]'] = $fan_page;
        }
        
        
        
        $form[$params->service . '_admin_add_fan_page'] = array(
            '#type' => 'container'
        );
        $form[$params->service . '_admin_add_fan_page']['page_profile'] = array(
                  '#markup' => '<div class="add-account-button-wrapper"><a href="javascript:void(0)" class="btn btn-add-account" onclick="Drupal.TBFeedFacebook.addPageProfile(\'' . $params->service . '\', this)">Add page</a></div>',
        );       
        $form[$params->service . '_fan_page_template'] = array(
            '#type' =>  'container',
            '#attributes' => array('style'  =>  array('display:none;'))
        );
        $fan_page['#default_value'] = NULL;
        $form[$params->service . '_fan_page_template']['fb-page-proflie'] = $fan_page;
    }
    /**
     * 
     * @param type $id
     */
    function fbGraph($id) {
          set_time_limit(60);
        $graph = "http://graph.facebook.com/$id";        
        $curl = new TBSocialFeed();
        $data = $curl->crawlPage($graph);        
        $data = json_decode($data);        
        return $data;
    }
    
    function getFeedTypes() {
        return array(
            0   =>  'All',
            1   =>  'Statuses',
            2   =>  'Feed',
            3   =>  'Posts',
            4   =>  'Home'
        );        
    }
    function getConnections($index) {
        $connections = array(
            1   =>  'statuses',
            2   =>  'feed',
            3   =>  'posts',
            4   =>  'home'
        );
        if (isset($connections[$index])) {
            return array($connections[$index]);
        } else {
            return $connections;
        }
    }
    function fbInitFacebook() {
        $config = array(
            'appId' =>  '588746591172770',
            'secret' => '6e7e8f2b5c59a80dbb9e845a54949d5b',
            'fileUpload' => FALSE
        );  
        $this->_facebook = new Facebook($config);        
    }
    
    /**
     * 
     * @param type $service
     * @param type $input
     * @param type $service_fields
     * @date 09-09-2013
     */
    public function extendedInfo($service, &$input, &$service_fields) {
        //Feed type
        if (isset($input[$service->service . '_admin_feed_type'])) {
            $service_fields['feed_type'] = intval($input[$service->service . '_admin_feed_type']);
        }
        //Optional settings
        if (isset($input[$service->service . '_admin_feed_has_image'])) {            
            $service_fields['feed_has_image'] = 1;
        } else {
            $service_fields['feed_has_image'] = 0;
        }
        $accounts = array(
            'feed_account'  =>  array(),
            'feed_account_name' =>  array(),
            'type'  =>  array()
        );
        //Admin Info 
//        if ($this->fbIsValidAccessToken()) {
//            $user_id = $this->_facebook->getUser();
//            $input[$service->service . '_admin_access_token'] = $this->fbGetExtenedAccessToken($user_id);           
//            $accounts['feed_account'][] = $user_id;
//            $accounts['feed_account_name'][] = $this->fbGetUsername();
//            $accounts['type'][]   = self::$USER_PAGE;
//        }
        //Page info
        if (!empty($input['facebook_fan_pages'])) {
            foreach ($input['facebook_fan_pages'] as $page) {
                if (!empty($page)) {                    
                    $id = $this->parsePageProfile($page);
                    $info = $this->fbGraph($id);  
                    $accounts['feed_account'][] = $info->id;
                    $accounts['feed_account_name'][] = $info->name;
                    $accounts['type'][] = self::$FAN_PAGE;
                }                
            }
        }
        $input[$service->service . '_feed_account'] = $accounts['feed_account'];
        $input[$service->service . '_feed_current_account'] = $accounts['feed_account'];
        $input[$service->service . '_feed_account_name'] = $accounts['feed_account_name'];
        $input[$service->service . '_type'] = $accounts['type'];
    }
    function fbGetUsername() {
        $info = $this->_facebook->api('/me');
        return $info['name'];
    }
    function fbGetAccount($id) {
        $info = $this->_facebook->api("/$id");
        return $info['username'];
    }
    function parsePageProfile($page) {
        $pattern = '/www\.facebook\.com\/(.*)/';
        preg_match_all($pattern, $page, $matches);        
        if (!empty($matches[1])) {
            return $matches[1][0];           
        } else {
            return NULL;
        }
    }
    function fbGetPageName($page) {
        
        $info = $this->_facebook->api("/$id_page");
        return $info['name'];
    }
    function fbGetAccessToken() {
        $access_token = $this->_facebook->getAccessToken();
        return $access_token;
    }
    function fbGetExtenedAccessToken($user_id) {
        $db_accounts = new Tb_Social_Feed_Accounts();
        $account = $db_accounts->select(array(), array(
            'account'   =>  $user_id
        ));
        $extend_access_token = NULL;
        if ($account) {
            $extend_access_token = $account->access_token;            
        } else {
            //Insert new account into tb_social_feed_accounts
            $this->_facebook->setExtendedAccessToken();  
            $extend_access_token = $this->_facebook->getAccessToken();            
        }
        $this->_facebook->setAccessToken($extend_access_token);        
        return $extend_access_token;
    }
   
    function fbIsValidAccessToken($account = 'me'){
        $flag = FALSE;
        $default_access_token = '588746591172770|6e7e8f2b5c59a80dbb9e845a54949d5b';
        
        if (!empty($this->configs['access_token'])) {
            $this->_facebook->setAccessToken($this->configs['access_token']);
        }
        
        $access_token = $this->_facebook->getAccessToken();
        if ($access_token != $default_access_token) {
            try {
                $this->_facebook->api("/$account");
                $flag = TRUE;
            } catch (FacebookApiException $e) {                
            }
        };
        return $flag;
    }
    function fbGetUserId() {
        return $this->_facebook->getUser();
    }
}
