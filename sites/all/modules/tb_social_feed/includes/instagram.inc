<?php

class TBInstagramFeed extends TBSocialFeed {

  function __construct($cfg = NULL) {
    $this->configs = array(
      'account' => $cfg ? $cfg['account'] : 'mashable',
      'limit' => $cfg['feed_limit']
    );
  }

  function getType() {
    return "Instagram";
  }

  public function getContent() {
    $url = "http://widget.stagram.com/rss/n/%s/";
    $url = sprintf($url, $this->configs['account']);
    $rss_item_tags = array('item', 'entry');
    $rss_tags = array(
      'title' => array('title'),
      'description' => array('description', 'content', 'summary'),
      'image' => array('url'),
      'link' => array('link', 'feedburner'),
      'category' => array('category'),
    );
    $content = $this->rssToArray($rss_item_tags, $rss_tags, $url);
    $results = array();
    $limit = $this->getFeedLimit($content);
    for ($i = 0; $i < $limit; $i++) {
      $row = $content[$i];
      $result = array();
      $result['title'] = ucwords($this->configs['account']);
      preg_match_all("'<strong>(.*?)</strong>'si", $row['description'][0], $match);
      $result['body'] = utf8_decode($match[1][0]);
      $img = preg_replace('/_6\.jpg/i', '_7.jpg', $row['image'][0]);
      $result['field_image'] = $result['feed_id'] = $img;
      $result['field_social_feed_type'] = $this->getType();
      $result['node_type'] = $this->getBundle();
      $result['field_reference_url'] = $result['reference_url'] = $row['link'][0];
      $result['created'] = $result['changed'] = 0;
      $result['url'] = $url;
      $result['service'] = 'instagram';
      $result['account'] = $this->configs['account'];
      $results[] = $result;
    }
    return $results;
  }

  function createAccountForm($account, &$form) {
    $form = array(
      '#type' => 'fieldset',
      '#title' => t("Account")
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
      '#title' => t("Search Keyword"),
      '#default_value' => isset($account->account) ? $account->account : "",
      '#description' => t("Enter your search keyword. Ex: mashable"),
      '#attributes' => array('class' => array('account')),
    );
    $form[$account->service . '_feed_current_account[]'] = array(
      '#type' => 'hidden',
      '#value' => isset($account->account) ? $account->account : ""
    );
    $form[$account->service . '_feed_account_verify'] = array(
      '#markup' => '<a href="javascript:void(0)" class="form-submit btn-verify-account" onclick="Drupal.TBFeed.verifyAccount(\'http://widget.stagram.com/rss/n/\', this)">' . t('Check feed') . '</a><br/>',
    );
    $form[$account->service . '_feed_account_delete'] = array(
      '#markup' => '<a href="javascript:void(0)" class="btn btn-delete-account" onclick="Drupal.TBFeed.deleteAccount(this)">' . t('Delete') . '</a>',
    );
  }

}
