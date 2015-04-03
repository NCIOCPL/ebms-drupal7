<?php
$view = new view();
$view->name = 'recent_activity';
$view->description = 'Show all existing messages.';
$view->tag = 'Message';
$view->base_table = 'message';
$view->human_name = 'Recent Activity';
$view->core = 7;
$view->api_version = '3.0';
$view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

/* Display: Defaults */
$handler = $view->new_display('default', 'Defaults', 'default');
$handler->display->display_options['title'] = 'Recent Activity';
$handler->display->display_options['use_more_always'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['cache']['type'] = 'none';
$handler->display->display_options['query']['type'] = 'views_query';
$handler->display->display_options['query']['options']['distinct'] = TRUE;
$handler->display->display_options['query']['options']['query_comment'] = FALSE;
$handler->display->display_options['exposed_form']['type'] = 'basic';
$handler->display->display_options['pager']['type'] = 'full';
$handler->display->display_options['pager']['options']['items_per_page'] = '10';
$handler->display->display_options['pager']['options']['offset'] = '0';
$handler->display->display_options['pager']['options']['id'] = '0';
$handler->display->display_options['pager']['options']['quantity'] = '9';
$handler->display->display_options['pager']['options']['expose']['items_per_page'] = TRUE;
$handler->display->display_options['pager']['options']['expose']['items_per_page_options_all'] = TRUE;
$handler->display->display_options['style_plugin'] = 'table';
$handler->display->display_options['style_options']['row_class_special'] = FALSE;
$handler->display->display_options['style_options']['columns'] = array(
  'timestamp' => 'timestamp',
  'message_render' => 'message_render',
  'name' => 'name',
);
$handler->display->display_options['style_options']['default'] = 'timestamp';
$handler->display->display_options['style_options']['info'] = array(
  'timestamp' => array(
    'sortable' => 1,
    'default_sort_order' => 'desc',
    'align' => '',
    'separator' => '',
    'empty_column' => 0,
  ),
  'message_render' => array(
    'align' => '',
    'separator' => '',
    'empty_column' => 0,
  ),
  'name' => array(
    'sortable' => 1,
    'default_sort_order' => 'asc',
    'align' => '',
    'separator' => '',
    'empty_column' => 0,
  ),
);
$handler->display->display_options['style_options']['sticky'] = TRUE;
/* Header: Global: Unfiltered text */
$handler->display->display_options['header']['area_text_custom']['id'] = 'area_text_custom';
$handler->display->display_options['header']['area_text_custom']['table'] = 'views';
$handler->display->display_options['header']['area_text_custom']['field'] = 'area_text_custom';
/* No results behavior: Global: Text area */
$handler->display->display_options['empty']['area']['id'] = 'area';
$handler->display->display_options['empty']['area']['table'] = 'views';
$handler->display->display_options['empty']['area']['field'] = 'area';
$handler->display->display_options['empty']['area']['empty'] = TRUE;
$handler->display->display_options['empty']['area']['content'] = 'No activity within the past 60 days.';
$handler->display->display_options['empty']['area']['format'] = 'filtered_html';
/* Relationship: Message: User uid */
$handler->display->display_options['relationships']['user']['id'] = 'user';
$handler->display->display_options['relationships']['user']['table'] = 'message';
$handler->display->display_options['relationships']['user']['field'] = 'user';
/* Field: Message: Timestamp */
$handler->display->display_options['fields']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['fields']['timestamp']['table'] = 'message';
$handler->display->display_options['fields']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['fields']['timestamp']['exclude'] = TRUE;
$handler->display->display_options['fields']['timestamp']['date_format'] = 'short';
/* Field: Message: Render message (Get text) */
$handler->display->display_options['fields']['message_render']['id'] = 'message_render';
$handler->display->display_options['fields']['message_render']['table'] = 'message';
$handler->display->display_options['fields']['message_render']['field'] = 'message_render';
$handler->display->display_options['fields']['message_render']['label'] = 'Events';
$handler->display->display_options['fields']['message_render']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['message_render']['hide_alter_empty'] = FALSE;
$handler->display->display_options['fields']['message_render']['partials'] = 0;
$handler->display->display_options['fields']['message_render']['partials_delta'] = '0';
/* Field: User: Name */
$handler->display->display_options['fields']['name']['id'] = 'name';
$handler->display->display_options['fields']['name']['table'] = 'users';
$handler->display->display_options['fields']['name']['field'] = 'name';
$handler->display->display_options['fields']['name']['relationship'] = 'user';
$handler->display->display_options['fields']['name']['label'] = 'User';
$handler->display->display_options['fields']['name']['exclude'] = TRUE;
/* Sort criterion: Message: Timestamp */
$handler->display->display_options['sorts']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['sorts']['timestamp']['table'] = 'message';
$handler->display->display_options['sorts']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['sorts']['timestamp']['order'] = 'DESC';
/* Filter criterion: Message: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'message';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'activity_new_articles' => 'activity_new_articles',
  'activity_new_packet' => 'activity_new_packet',
);
$handler->display->display_options['filters']['type']['expose']['operator_id'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['label'] = 'Message type';
$handler->display->display_options['filters']['type']['expose']['operator'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['identifier'] = 'type';
$handler->display->display_options['filters']['type']['expose']['remember_roles'] = array(
  2 => '2',
  9 => 0,
  1 => 0,
  5 => 0,
  4 => 0,
  6 => 0,
  8 => 0,
  7 => 0,
  3 => 0,
);
/* Filter criterion: Message: Timestamp */
$handler->display->display_options['filters']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['table'] = 'message';
$handler->display->display_options['filters']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['operator'] = '>=';
$handler->display->display_options['filters']['timestamp']['value']['value'] = '-60 days';
$handler->display->display_options['filters']['timestamp']['value']['type'] = 'offset';

/* Display: Meeting Activity Page */
$handler = $view->new_display('page', 'Meeting Activity Page', 'meeting_activity_page');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['header'] = FALSE;
/* Header: Global: Text area */
$handler->display->display_options['header']['area']['id'] = 'area';
$handler->display->display_options['header']['area']['table'] = 'views';
$handler->display->display_options['header']['area']['field'] = 'area';
$handler->display->display_options['header']['area']['content'] = '<h2>Recent Meeting Activity</h2>
<p>Meeting activity from the last 60 days</p>';
$handler->display->display_options['header']['area']['format'] = 'full_html';
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Message: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'message';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'activity_agenda_published' => 'activity_agenda_published',
  'activity_cancelled_event' => 'activity_cancelled_event',
  'activity_changed_event' => 'activity_changed_event',
  'activity_event_type' => 'activity_event_type',
  'activity_new_event' => 'activity_new_event',
);
$handler->display->display_options['filters']['type']['expose']['operator_id'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['label'] = 'Message type';
$handler->display->display_options['filters']['type']['expose']['operator'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['identifier'] = 'type';
$handler->display->display_options['filters']['type']['expose']['remember_roles'] = array(
  2 => '2',
  9 => 0,
  1 => 0,
  5 => 0,
  4 => 0,
  6 => 0,
  8 => 0,
  7 => 0,
  3 => 0,
);
/* Filter criterion: Message: Timestamp */
$handler->display->display_options['filters']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['table'] = 'message';
$handler->display->display_options['filters']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['operator'] = '>=';
$handler->display->display_options['filters']['timestamp']['value']['value'] = '-60 days';
$handler->display->display_options['filters']['timestamp']['value']['type'] = 'offset';
$handler->display->display_options['path'] = 'recent-meeting-activity';
$handler->display->display_options['menu']['title'] = 'Recent Activity';
$handler->display->display_options['menu']['description'] = 'All activity messages from the last 30 days.';
$handler->display->display_options['menu']['weight'] = '0';
$handler->display->display_options['menu']['name'] = 'management';
$handler->display->display_options['menu']['context'] = 0;

/* Display: Meeting Activity Block */
$handler = $view->new_display('block', 'Meeting Activity Block', 'meeting_activity_block');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['use_more'] = FALSE;
$handler->display->display_options['use_more'] = TRUE;
$handler->display->display_options['defaults']['use_more_always'] = FALSE;
$handler->display->display_options['defaults']['use_more_always'] = FALSE;
$handler->display->display_options['use_more_always'] = FALSE;
$handler->display->display_options['defaults']['use_more_text'] = FALSE;
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'some';
$handler->display->display_options['pager']['options']['items_per_page'] = '5';
$handler->display->display_options['pager']['options']['offset'] = '0';
$handler->display->display_options['defaults']['fields'] = FALSE;
/* Field: Message: Timestamp */
$handler->display->display_options['fields']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['fields']['timestamp']['table'] = 'message';
$handler->display->display_options['fields']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['fields']['timestamp']['exclude'] = TRUE;
$handler->display->display_options['fields']['timestamp']['date_format'] = 'short';
/* Field: Message: Render message (Get text) */
$handler->display->display_options['fields']['message_render']['id'] = 'message_render';
$handler->display->display_options['fields']['message_render']['table'] = 'message';
$handler->display->display_options['fields']['message_render']['field'] = 'message_render';
$handler->display->display_options['fields']['message_render']['label'] = '';
$handler->display->display_options['fields']['message_render']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['message_render']['hide_alter_empty'] = FALSE;
$handler->display->display_options['fields']['message_render']['partials'] = 0;
$handler->display->display_options['fields']['message_render']['partials_delta'] = '0';
/* Field: User: Name */
$handler->display->display_options['fields']['name']['id'] = 'name';
$handler->display->display_options['fields']['name']['table'] = 'users';
$handler->display->display_options['fields']['name']['field'] = 'name';
$handler->display->display_options['fields']['name']['relationship'] = 'user';
$handler->display->display_options['fields']['name']['label'] = 'User';
$handler->display->display_options['fields']['name']['exclude'] = TRUE;
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Message: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'message';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'activity_agenda_published' => 'activity_agenda_published',
  'activity_cancelled_event' => 'activity_cancelled_event',
  'activity_changed_event' => 'activity_changed_event',
  'activity_event_type' => 'activity_event_type',
  'activity_new_event' => 'activity_new_event',
);
$handler->display->display_options['filters']['type']['expose']['operator_id'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['label'] = 'Message type';
$handler->display->display_options['filters']['type']['expose']['operator'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['identifier'] = 'type';
$handler->display->display_options['filters']['type']['expose']['remember_roles'] = array(
  2 => '2',
  9 => 0,
  1 => 0,
  5 => 0,
  4 => 0,
  6 => 0,
  8 => 0,
  7 => 0,
  3 => 0,
);
/* Filter criterion: Message: Timestamp */
$handler->display->display_options['filters']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['table'] = 'message';
$handler->display->display_options['filters']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['operator'] = '>=';
$handler->display->display_options['filters']['timestamp']['value']['value'] = '-60 days';
$handler->display->display_options['filters']['timestamp']['value']['type'] = 'offset';

/* Display: Literature Activity Page */
$handler = $view->new_display('page', 'Literature Activity Page', 'literature_activity_page');
$handler->display->display_options['defaults']['header'] = FALSE;
/* Header: Global: Unfiltered text */
$handler->display->display_options['header']['area_text_custom']['id'] = 'area_text_custom';
$handler->display->display_options['header']['area_text_custom']['table'] = 'views';
$handler->display->display_options['header']['area_text_custom']['field'] = 'area_text_custom';
$handler->display->display_options['header']['area_text_custom']['content'] = '<h2>Recent Literature Activity</h2>
<p>Literature activity from the last 60 days</p>';
$handler->display->display_options['defaults']['empty'] = FALSE;
/* No results behavior: Global: Text area */
$handler->display->display_options['empty']['area']['id'] = 'area';
$handler->display->display_options['empty']['area']['table'] = 'views';
$handler->display->display_options['empty']['area']['field'] = 'area';
$handler->display->display_options['empty']['area']['empty'] = TRUE;
$handler->display->display_options['empty']['area']['content'] = 'No activity within the past 30 days.';
$handler->display->display_options['empty']['area']['format'] = 'filtered_html';
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Message: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'message';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'activity_new_articles' => 'activity_new_articles',
  'activity_new_packet' => 'activity_new_packet',
);
$handler->display->display_options['filters']['type']['expose']['operator_id'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['label'] = 'Message type';
$handler->display->display_options['filters']['type']['expose']['operator'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['identifier'] = 'type';
$handler->display->display_options['filters']['type']['expose']['remember_roles'] = array(
  2 => '2',
  9 => 0,
  1 => 0,
  5 => 0,
  4 => 0,
  6 => 0,
  8 => 0,
  7 => 0,
  3 => 0,
);
/* Filter criterion: Message: Timestamp */
$handler->display->display_options['filters']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['table'] = 'message';
$handler->display->display_options['filters']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['operator'] = '>=';
$handler->display->display_options['filters']['timestamp']['value']['value'] = '-30 days';
$handler->display->display_options['filters']['timestamp']['value']['type'] = 'offset';
$handler->display->display_options['path'] = 'recent-literature-activity';

/* Display: Literature Activity Block */
$handler = $view->new_display('block', 'Literature Activity Block', 'literature_activity_block');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['use_more'] = FALSE;
$handler->display->display_options['use_more'] = TRUE;
$handler->display->display_options['defaults']['use_more_always'] = FALSE;
$handler->display->display_options['defaults']['use_more_always'] = FALSE;
$handler->display->display_options['use_more_always'] = FALSE;
$handler->display->display_options['defaults']['use_more_text'] = FALSE;
$handler->display->display_options['defaults']['link_display'] = FALSE;
$handler->display->display_options['link_display'] = 'literature_activity_page';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'some';
$handler->display->display_options['pager']['options']['items_per_page'] = '5';
$handler->display->display_options['pager']['options']['offset'] = '0';
$handler->display->display_options['defaults']['empty'] = FALSE;
/* No results behavior: Global: Text area */
$handler->display->display_options['empty']['area']['id'] = 'area';
$handler->display->display_options['empty']['area']['table'] = 'views';
$handler->display->display_options['empty']['area']['field'] = 'area';
$handler->display->display_options['empty']['area']['empty'] = TRUE;
$handler->display->display_options['empty']['area']['content'] = 'No activity within the past 30 days.';
$handler->display->display_options['empty']['area']['format'] = 'filtered_html';
$handler->display->display_options['defaults']['fields'] = FALSE;
/* Field: Message: Timestamp */
$handler->display->display_options['fields']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['fields']['timestamp']['table'] = 'message';
$handler->display->display_options['fields']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['fields']['timestamp']['exclude'] = TRUE;
$handler->display->display_options['fields']['timestamp']['date_format'] = 'short';
/* Field: Message: Render message (Get text) */
$handler->display->display_options['fields']['message_render']['id'] = 'message_render';
$handler->display->display_options['fields']['message_render']['table'] = 'message';
$handler->display->display_options['fields']['message_render']['field'] = 'message_render';
$handler->display->display_options['fields']['message_render']['label'] = '';
$handler->display->display_options['fields']['message_render']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['message_render']['hide_alter_empty'] = FALSE;
$handler->display->display_options['fields']['message_render']['partials'] = 0;
$handler->display->display_options['fields']['message_render']['partials_delta'] = '0';
/* Field: User: Name */
$handler->display->display_options['fields']['name']['id'] = 'name';
$handler->display->display_options['fields']['name']['table'] = 'users';
$handler->display->display_options['fields']['name']['field'] = 'name';
$handler->display->display_options['fields']['name']['relationship'] = 'user';
$handler->display->display_options['fields']['name']['label'] = 'User';
$handler->display->display_options['fields']['name']['exclude'] = TRUE;
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Message: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'message';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'activity_new_articles' => 'activity_new_articles',
  'activity_new_packet' => 'activity_new_packet',
);
$handler->display->display_options['filters']['type']['expose']['operator_id'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['label'] = 'Message type';
$handler->display->display_options['filters']['type']['expose']['operator'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['identifier'] = 'type';
$handler->display->display_options['filters']['type']['expose']['remember_roles'] = array(
  2 => '2',
  9 => 0,
  1 => 0,
  5 => 0,
  4 => 0,
  6 => 0,
  8 => 0,
  7 => 0,
  3 => 0,
);
/* Filter criterion: Message: Timestamp */
$handler->display->display_options['filters']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['table'] = 'message';
$handler->display->display_options['filters']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['operator'] = '>=';
$handler->display->display_options['filters']['timestamp']['value']['value'] = '-30 days';
$handler->display->display_options['filters']['timestamp']['value']['type'] = 'offset';

/* Display: Document Activity Page */
$handler = $view->new_display('page', 'Document Activity Page', 'document_activity_page');
$handler->display->display_options['defaults']['header'] = FALSE;
/* Header: Global: Unfiltered text */
$handler->display->display_options['header']['area_text_custom']['id'] = 'area_text_custom';
$handler->display->display_options['header']['area_text_custom']['table'] = 'views';
$handler->display->display_options['header']['area_text_custom']['field'] = 'area_text_custom';
$handler->display->display_options['header']['area_text_custom']['content'] = '<h2>Recent Document Activity</h2>
<p>Document activity from the last 60 days</p>';
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Message: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'message';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'activity_new_summary' => 'activity_new_summary',
);
$handler->display->display_options['filters']['type']['expose']['operator_id'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['label'] = 'Message type';
$handler->display->display_options['filters']['type']['expose']['operator'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['identifier'] = 'type';
$handler->display->display_options['filters']['type']['expose']['remember_roles'] = array(
  2 => '2',
  9 => 0,
  1 => 0,
  5 => 0,
  4 => 0,
  6 => 0,
  8 => 0,
  7 => 0,
  3 => 0,
);
/* Filter criterion: Message: Timestamp */
$handler->display->display_options['filters']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['table'] = 'message';
$handler->display->display_options['filters']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['operator'] = '>=';
$handler->display->display_options['filters']['timestamp']['value']['value'] = '-60 days';
$handler->display->display_options['filters']['timestamp']['value']['type'] = 'offset';
$handler->display->display_options['path'] = 'recent-document-activity';

/* Display: Document Activity Block */
$handler = $view->new_display('block', 'Document Activity Block', 'document_activity_block');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['use_more'] = FALSE;
$handler->display->display_options['use_more'] = TRUE;
$handler->display->display_options['defaults']['use_more_always'] = FALSE;
$handler->display->display_options['defaults']['use_more_always'] = FALSE;
$handler->display->display_options['use_more_always'] = FALSE;
$handler->display->display_options['defaults']['use_more_text'] = FALSE;
$handler->display->display_options['defaults']['link_display'] = FALSE;
$handler->display->display_options['link_display'] = 'document_activity_page';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'some';
$handler->display->display_options['pager']['options']['items_per_page'] = '5';
$handler->display->display_options['pager']['options']['offset'] = '0';
$handler->display->display_options['defaults']['fields'] = FALSE;
/* Field: Message: Timestamp */
$handler->display->display_options['fields']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['fields']['timestamp']['table'] = 'message';
$handler->display->display_options['fields']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['fields']['timestamp']['exclude'] = TRUE;
$handler->display->display_options['fields']['timestamp']['date_format'] = 'short';
/* Field: Message: Render message (Get text) */
$handler->display->display_options['fields']['message_render']['id'] = 'message_render';
$handler->display->display_options['fields']['message_render']['table'] = 'message';
$handler->display->display_options['fields']['message_render']['field'] = 'message_render';
$handler->display->display_options['fields']['message_render']['label'] = '';
$handler->display->display_options['fields']['message_render']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['message_render']['hide_alter_empty'] = FALSE;
$handler->display->display_options['fields']['message_render']['partials'] = 0;
$handler->display->display_options['fields']['message_render']['partials_delta'] = '0';
/* Field: User: Name */
$handler->display->display_options['fields']['name']['id'] = 'name';
$handler->display->display_options['fields']['name']['table'] = 'users';
$handler->display->display_options['fields']['name']['field'] = 'name';
$handler->display->display_options['fields']['name']['relationship'] = 'user';
$handler->display->display_options['fields']['name']['label'] = 'User';
$handler->display->display_options['fields']['name']['exclude'] = TRUE;
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Message: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'message';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'activity_new_summary' => 'activity_new_summary',
);
$handler->display->display_options['filters']['type']['expose']['operator_id'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['label'] = 'Message type';
$handler->display->display_options['filters']['type']['expose']['operator'] = 'type_op';
$handler->display->display_options['filters']['type']['expose']['identifier'] = 'type';
$handler->display->display_options['filters']['type']['expose']['remember_roles'] = array(
  2 => '2',
  9 => 0,
  1 => 0,
  5 => 0,
  4 => 0,
  6 => 0,
  8 => 0,
  7 => 0,
  3 => 0,
);
/* Filter criterion: Message: Timestamp */
$handler->display->display_options['filters']['timestamp']['id'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['table'] = 'message';
$handler->display->display_options['filters']['timestamp']['field'] = 'timestamp';
$handler->display->display_options['filters']['timestamp']['operator'] = '>=';
$handler->display->display_options['filters']['timestamp']['value']['value'] = '-60 days';
$handler->display->display_options['filters']['timestamp']['value']['type'] = 'offset';
