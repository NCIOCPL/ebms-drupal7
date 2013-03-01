<?php
$view = new view();
$view->name = 'ebms_forum';
$view->description = 'This view formats a forum\'s list of topics and provides the ability to select archived or non-archived topics.';
$view->tag = 'default';
$view->base_table = 'node';
$view->human_name = 'EBMS Forum';
$view->core = 7;
$view->api_version = '3.0';
$view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

/* Display: Master */
$handler = $view->new_display('default', 'Master', 'default');
$handler->display->display_options['title'] = '%1';
$handler->display->display_options['use_more_always'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['cache']['type'] = 'none';
$handler->display->display_options['query']['type'] = 'views_query';
$handler->display->display_options['exposed_form']['type'] = 'basic';
$handler->display->display_options['pager']['type'] = 'full';
$handler->display->display_options['pager']['options']['items_per_page'] = '10';
$handler->display->display_options['style_plugin'] = 'default';
$handler->display->display_options['style_options']['row_class'] = 'forum-topic-on-forum';
$handler->display->display_options['row_plugin'] = 'fields';
/* No results behavior: Global: Text area */
$handler->display->display_options['empty']['area']['id'] = 'area';
$handler->display->display_options['empty']['area']['table'] = 'views';
$handler->display->display_options['empty']['area']['field'] = 'area';
$handler->display->display_options['empty']['area']['label'] = 'No Results';
$handler->display->display_options['empty']['area']['empty'] = TRUE;
$handler->display->display_options['empty']['area']['content'] = 'There are no topics available for this forum.';
$handler->display->display_options['empty']['area']['format'] = 'filtered_html';
/* Relationship: Content: Author */
$handler->display->display_options['relationships']['uid']['id'] = 'uid';
$handler->display->display_options['relationships']['uid']['table'] = 'node';
$handler->display->display_options['relationships']['uid']['field'] = 'uid';
/* Relationship: Comment: Last Comment */
$handler->display->display_options['relationships']['cid']['id'] = 'cid';
$handler->display->display_options['relationships']['cid']['table'] = 'node_comment_statistics';
$handler->display->display_options['relationships']['cid']['field'] = 'cid';
/* Field: Content: Title */
$handler->display->display_options['fields']['title']['id'] = 'title';
$handler->display->display_options['fields']['title']['table'] = 'node';
$handler->display->display_options['fields']['title']['field'] = 'title';
$handler->display->display_options['fields']['title']['label'] = '';
$handler->display->display_options['fields']['title']['alter']['word_boundary'] = FALSE;
$handler->display->display_options['fields']['title']['alter']['ellipsis'] = FALSE;
$handler->display->display_options['fields']['title']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['title']['element_wrapper_class'] = 'forum-title';
/* Field: Content: Body */
$handler->display->display_options['fields']['body']['id'] = 'body';
$handler->display->display_options['fields']['body']['table'] = 'field_data_body';
$handler->display->display_options['fields']['body']['field'] = 'body';
$handler->display->display_options['fields']['body']['label'] = '';
$handler->display->display_options['fields']['body']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['body']['element_wrapper_class'] = 'forum-description';
$handler->display->display_options['fields']['body']['empty'] = '<p>No Summary Available</p>';
$handler->display->display_options['fields']['body']['type'] = 'text_summary_or_trimmed';
$handler->display->display_options['fields']['body']['settings'] = array(
  'trim_length' => '600',
);
/* Field: User: Name */
$handler->display->display_options['fields']['name']['id'] = 'name';
$handler->display->display_options['fields']['name']['table'] = 'users';
$handler->display->display_options['fields']['name']['field'] = 'name';
$handler->display->display_options['fields']['name']['relationship'] = 'uid';
$handler->display->display_options['fields']['name']['label'] = '';
$handler->display->display_options['fields']['name']['exclude'] = TRUE;
$handler->display->display_options['fields']['name']['element_label_colon'] = FALSE;
/* Field: Content: Post date */
$handler->display->display_options['fields']['created']['id'] = 'created';
$handler->display->display_options['fields']['created']['table'] = 'node';
$handler->display->display_options['fields']['created']['field'] = 'created';
$handler->display->display_options['fields']['created']['label'] = '';
$handler->display->display_options['fields']['created']['exclude'] = TRUE;
$handler->display->display_options['fields']['created']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['created']['date_format'] = 'medium';
/* Field: Global: Custom text */
$handler->display->display_options['fields']['nothing']['id'] = 'nothing';
$handler->display->display_options['fields']['nothing']['table'] = 'views';
$handler->display->display_options['fields']['nothing']['field'] = 'nothing';
$handler->display->display_options['fields']['nothing']['label'] = 'Started by';
$handler->display->display_options['fields']['nothing']['alter']['text'] = '[name] | [created]';
$handler->display->display_options['fields']['nothing']['element_wrapper_class'] = 'forum-topic-started-info';
/* Field: Recent Activity */
$handler->display->display_options['fields']['rendered_entity']['id'] = 'rendered_entity';
$handler->display->display_options['fields']['rendered_entity']['table'] = 'views_entity_comment';
$handler->display->display_options['fields']['rendered_entity']['field'] = 'rendered_entity';
$handler->display->display_options['fields']['rendered_entity']['relationship'] = 'cid';
$handler->display->display_options['fields']['rendered_entity']['ui_name'] = 'Recent Activity';
$handler->display->display_options['fields']['rendered_entity']['label'] = '';
$handler->display->display_options['fields']['rendered_entity']['alter']['alter_text'] = TRUE;
$handler->display->display_options['fields']['rendered_entity']['alter']['text'] = '<div class="forum-topic-recent-activity-words">Recent Activity:</div>
<div class="forum-topic-recent-activity-activity">[rendered_entity]</div>';
$handler->display->display_options['fields']['rendered_entity']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['rendered_entity']['element_wrapper_class'] = 'forum-topic-recent-activity';
$handler->display->display_options['fields']['rendered_entity']['link_to_entity'] = 1;
$handler->display->display_options['fields']['rendered_entity']['display'] = 'view';
$handler->display->display_options['fields']['rendered_entity']['view_mode'] = 'full';
/* Sort criterion: Content: Post date */
$handler->display->display_options['sorts']['created']['id'] = 'created';
$handler->display->display_options['sorts']['created']['table'] = 'node';
$handler->display->display_options['sorts']['created']['field'] = 'created';
$handler->display->display_options['sorts']['created']['order'] = 'DESC';
/* Contextual filter: Content: Forums (taxonomy_forums) */
$handler->display->display_options['arguments']['taxonomy_forums_tid']['id'] = 'taxonomy_forums_tid';
$handler->display->display_options['arguments']['taxonomy_forums_tid']['table'] = 'field_data_taxonomy_forums';
$handler->display->display_options['arguments']['taxonomy_forums_tid']['field'] = 'taxonomy_forums_tid';
$handler->display->display_options['arguments']['taxonomy_forums_tid']['default_action'] = 'empty';
$handler->display->display_options['arguments']['taxonomy_forums_tid']['default_argument_type'] = 'fixed';
$handler->display->display_options['arguments']['taxonomy_forums_tid']['summary']['number_of_records'] = '0';
$handler->display->display_options['arguments']['taxonomy_forums_tid']['summary']['format'] = 'default_summary';
$handler->display->display_options['arguments']['taxonomy_forums_tid']['summary_options']['items_per_page'] = '25';
$handler->display->display_options['arguments']['taxonomy_forums_tid']['specify_validation'] = TRUE;
$handler->display->display_options['arguments']['taxonomy_forums_tid']['validate']['type'] = 'taxonomy_term';
$handler->display->display_options['arguments']['taxonomy_forums_tid']['validate_options']['vocabularies'] = array(
  'forums' => 'forums',
);
$handler->display->display_options['arguments']['taxonomy_forums_tid']['validate']['fail'] = 'access denied';
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = 1;
$handler->display->display_options['filters']['status']['group'] = 1;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'forum' => 'forum',
);
/* Filter criterion: Field: Archived (field_archived) */
$handler->display->display_options['filters']['field_archived_value']['id'] = 'field_archived_value';
$handler->display->display_options['filters']['field_archived_value']['table'] = 'field_data_field_archived';
$handler->display->display_options['filters']['field_archived_value']['field'] = 'field_archived_value';
$handler->display->display_options['filters']['field_archived_value']['value'] = array(
  0 => '0',
);

/* Display: Topics */
$handler = $view->new_display('page', 'Topics', 'topics');
$handler->display->display_options['defaults']['title'] = FALSE;
$handler->display->display_options['title'] = '%1';
$handler->display->display_options['display_description'] = 'A list of unarchived topics for the given forum.';
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['path'] = 'forum/%/topics';

/* Display: Archived Topics */
$handler = $view->new_display('page', 'Archived Topics', 'archived');
$handler->display->display_options['defaults']['title'] = FALSE;
$handler->display->display_options['title'] = '%1 (Archived Topics)';
$handler->display->display_options['display_description'] = 'A list of archived topics for the given forum.';
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['empty'] = FALSE;
/* No results behavior: Global: Text area */
$handler->display->display_options['empty']['area']['id'] = 'area';
$handler->display->display_options['empty']['area']['table'] = 'views';
$handler->display->display_options['empty']['area']['field'] = 'area';
$handler->display->display_options['empty']['area']['label'] = 'No Results';
$handler->display->display_options['empty']['area']['empty'] = TRUE;
$handler->display->display_options['empty']['area']['content'] = 'There are no archived topics available for this forum.';
$handler->display->display_options['empty']['area']['format'] = 'filtered_html';
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = 1;
$handler->display->display_options['filters']['status']['group'] = 1;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'forum' => 'forum',
);
/* Filter criterion: Field: Archived (field_archived) */
$handler->display->display_options['filters']['field_archived_value']['id'] = 'field_archived_value';
$handler->display->display_options['filters']['field_archived_value']['table'] = 'field_data_field_archived';
$handler->display->display_options['filters']['field_archived_value']['field'] = 'field_archived_value';
$handler->display->display_options['filters']['field_archived_value']['value'] = array(
  1 => '1',
);
$handler->display->display_options['path'] = 'forum/%/archived';
