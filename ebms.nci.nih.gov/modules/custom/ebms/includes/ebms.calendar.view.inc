<?php

$view = new view();
$view->name = 'event_calendar';
$view->description = '';
$view->tag = 'Calendar';
$view->base_table = 'node';
$view->human_name = 'Event Calendar';
$view->core = 7;
$view->api_version = '3.0';
$view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

/* Display: Master */
$handler = $view->new_display('default', 'Master', 'default');
$handler->display->display_options['title'] = 'Calendar';
$handler->display->display_options['use_more_always'] = FALSE;
$handler->display->display_options['link_display'] = 'month';
$handler->display->display_options['access']['type'] = 'none';
$handler->display->display_options['cache']['type'] = 'none';
$handler->display->display_options['query']['type'] = 'views_query';
$handler->display->display_options['query']['options']['distinct'] = TRUE;
$handler->display->display_options['exposed_form']['type'] = 'basic';
$handler->display->display_options['pager']['type'] = 'date_views_pager';
$handler->display->display_options['pager']['options']['date_id'] = 'month';
$handler->display->display_options['style_plugin'] = 'calendar_style';
$handler->display->display_options['style_options']['name_size'] = '3';
$handler->display->display_options['style_options']['with_weekno'] = '0';
$handler->display->display_options['style_options']['multiday_theme'] = '1';
$handler->display->display_options['style_options']['theme_style'] = '1';
$handler->display->display_options['style_options']['max_items'] = '0';
$handler->display->display_options['style_options']['groupby_times'] = 'half';
$handler->display->display_options['row_plugin'] = 'calendar_entity';
/* Field: Content: Event Date */
$handler->display->display_options['fields']['field_datespan']['id'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['table'] = 'field_data_field_datespan';
$handler->display->display_options['fields']['field_datespan']['field'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['label'] = '';
$handler->display->display_options['fields']['field_datespan']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['hide_alter_empty'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['settings'] = array(
  'format_type' => 'short',
  'fromto' => 'both',
  'multiple_number' => '',
  'multiple_from' => '',
  'multiple_to' => '',
  'show_repeat_rule' => '',
);
$handler->display->display_options['fields']['field_datespan']['group_rows'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['delta_offset'] = '0';
/* Field: Content: Event Status */
$handler->display->display_options['fields']['field_event_status']['id'] = 'field_event_status';
$handler->display->display_options['fields']['field_event_status']['table'] = 'field_data_field_event_status';
$handler->display->display_options['fields']['field_event_status']['field'] = 'field_event_status';
$handler->display->display_options['fields']['field_event_status']['label'] = '';
$handler->display->display_options['fields']['field_event_status']['exclude'] = TRUE;
$handler->display->display_options['fields']['field_event_status']['alter']['text'] = '<?php
if(\'[field_event_status-value]\' == \'cancelled\') {
    print(\'(Cancelled)\');
else { print(\'On Calendar\'); }
?>';
$handler->display->display_options['fields']['field_event_status']['element_label_colon'] = FALSE;
/* Field: Content: Title */
$handler->display->display_options['fields']['title']['id'] = 'title';
$handler->display->display_options['fields']['title']['table'] = 'node';
$handler->display->display_options['fields']['title']['field'] = 'title';
$handler->display->display_options['fields']['title']['label'] = '';
$handler->display->display_options['fields']['title']['alter']['word_boundary'] = FALSE;
$handler->display->display_options['fields']['title']['alter']['ellipsis'] = FALSE;
$handler->display->display_options['fields']['title']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['title']['hide_alter_empty'] = FALSE;
/* Sort criterion: Content: Event Date -  start date (field_datespan) */
$handler->display->display_options['sorts']['field_datespan_value']['id'] = 'field_datespan_value';
$handler->display->display_options['sorts']['field_datespan_value']['table'] = 'field_data_field_datespan';
$handler->display->display_options['sorts']['field_datespan_value']['field'] = 'field_datespan_value';
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'ebms_event' => 'ebms_event',
);
$handler->display->display_options['filters']['type']['group'] = 1;
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = '1';
$handler->display->display_options['filters']['status']['group'] = 1;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;

/* Display: Month All */
$handler = $view->new_display('page', 'Month All', 'month_all');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['access']['perm'] = 'view all events';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'date_views_pager';
$handler->display->display_options['pager']['options']['date_id'] = 'month';
$handler->display->display_options['pager']['options']['link_format'] = 'clean';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'calendar_style';
$handler->display->display_options['style_options']['name_size'] = '3';
$handler->display->display_options['style_options']['mini'] = '0';
$handler->display->display_options['style_options']['with_weekno'] = '0';
$handler->display->display_options['style_options']['multiday_theme'] = '1';
$handler->display->display_options['style_options']['theme_style'] = '1';
$handler->display->display_options['style_options']['max_items'] = '0';
$handler->display->display_options['style_options']['groupby_times'] = 'half';
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'calendar_entity';
$handler->display->display_options['row_options']['colors']['legend'] = 'type';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['arguments'] = FALSE;
/* Contextual filter: Date: Date (node) */
$handler->display->display_options['arguments']['date_argument']['id'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['table'] = 'node';
$handler->display->display_options['arguments']['date_argument']['field'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['default_action'] = 'default';
$handler->display->display_options['arguments']['date_argument']['default_argument_type'] = 'date';
$handler->display->display_options['arguments']['date_argument']['summary']['format'] = 'default_summary';
$handler->display->display_options['arguments']['date_argument']['add_delta'] = 'yes';
$handler->display->display_options['arguments']['date_argument']['date_fields'] = array(
  'field_data_field_datespan.field_datespan_value' => 'field_data_field_datespan.field_datespan_value',
);
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'ebms_event' => 'ebms_event',
);
$handler->display->display_options['filters']['type']['group'] = 1;
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = '1';
$handler->display->display_options['filters']['status']['group'] = 1;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
/* Filter criterion: Content: Event Category (field_event_category) */
$handler->display->display_options['filters']['field_event_category_value']['id'] = 'field_event_category_value';
$handler->display->display_options['filters']['field_event_category_value']['table'] = 'field_data_field_event_category';
$handler->display->display_options['filters']['field_event_category_value']['field'] = 'field_event_category_value';
$handler->display->display_options['filters']['field_event_category_value']['value'] = array(
  'board' => 'board',
);
$handler->display->display_options['path'] = 'calendar/all';
$handler->display->display_options['menu']['title'] = 'Month';
$handler->display->display_options['menu']['weight'] = '0';
$handler->display->display_options['menu']['context'] = 0;
$handler->display->display_options['tab_options']['type'] = 'normal';
$handler->display->display_options['tab_options']['title'] = 'Calendar';
$handler->display->display_options['tab_options']['weight'] = '0';

/* Display: Week All */
$handler = $view->new_display('page', 'Week All', 'week_all');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['access']['perm'] = 'view all events';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'date_views_pager';
$handler->display->display_options['pager']['options']['date_id'] = 'week';
$handler->display->display_options['pager']['options']['link_format'] = 'clean';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'calendar_style';
$handler->display->display_options['style_options']['calendar_type'] = 'week';
$handler->display->display_options['style_options']['name_size'] = '3';
$handler->display->display_options['style_options']['mini'] = '0';
$handler->display->display_options['style_options']['with_weekno'] = '0';
$handler->display->display_options['style_options']['multiday_theme'] = '1';
$handler->display->display_options['style_options']['theme_style'] = '1';
$handler->display->display_options['style_options']['max_items'] = '0';
$handler->display->display_options['style_options']['groupby_times'] = 'half';
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'calendar_entity';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['arguments'] = FALSE;
/* Contextual filter: Date: Date (node) */
$handler->display->display_options['arguments']['date_argument']['id'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['table'] = 'node';
$handler->display->display_options['arguments']['date_argument']['field'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['default_action'] = 'default';
$handler->display->display_options['arguments']['date_argument']['default_argument_type'] = 'date';
$handler->display->display_options['arguments']['date_argument']['summary']['format'] = 'default_summary';
$handler->display->display_options['arguments']['date_argument']['granularity'] = 'week';
$handler->display->display_options['arguments']['date_argument']['add_delta'] = 'yes';
$handler->display->display_options['arguments']['date_argument']['date_fields'] = array(
  'field_data_field_datespan.field_datespan_value' => 'field_data_field_datespan.field_datespan_value',
);
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'ebms_event' => 'ebms_event',
);
$handler->display->display_options['filters']['type']['group'] = 1;
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = '1';
$handler->display->display_options['filters']['status']['group'] = 1;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
/* Filter criterion: Content: Event Category (field_event_category) */
$handler->display->display_options['filters']['field_event_category_value']['id'] = 'field_event_category_value';
$handler->display->display_options['filters']['field_event_category_value']['table'] = 'field_data_field_event_category';
$handler->display->display_options['filters']['field_event_category_value']['field'] = 'field_event_category_value';
$handler->display->display_options['filters']['field_event_category_value']['value'] = array(
  'board' => 'board',
);
$handler->display->display_options['path'] = 'calendar/week/all';
$handler->display->display_options['menu']['title'] = 'Week';
$handler->display->display_options['menu']['weight'] = '2';
$handler->display->display_options['menu']['context'] = 0;

/* Display: Day All */
$handler = $view->new_display('page', 'Day All', 'day_all');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['access']['perm'] = 'view all events';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'date_views_pager';
$handler->display->display_options['pager']['options']['date_id'] = 'day';
$handler->display->display_options['pager']['options']['link_format'] = 'clean';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'calendar_style';
$handler->display->display_options['style_options']['calendar_type'] = 'day';
$handler->display->display_options['style_options']['name_size'] = '3';
$handler->display->display_options['style_options']['mini'] = '0';
$handler->display->display_options['style_options']['with_weekno'] = '0';
$handler->display->display_options['style_options']['multiday_theme'] = '1';
$handler->display->display_options['style_options']['theme_style'] = '1';
$handler->display->display_options['style_options']['max_items'] = '0';
$handler->display->display_options['style_options']['groupby_times'] = 'half';
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'calendar_entity';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['empty'] = FALSE;
$handler->display->display_options['defaults']['arguments'] = FALSE;
/* Contextual filter: Date: Date (node) */
$handler->display->display_options['arguments']['date_argument']['id'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['table'] = 'node';
$handler->display->display_options['arguments']['date_argument']['field'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['default_action'] = 'default';
$handler->display->display_options['arguments']['date_argument']['default_argument_type'] = 'date';
$handler->display->display_options['arguments']['date_argument']['summary']['format'] = 'default_summary';
$handler->display->display_options['arguments']['date_argument']['granularity'] = 'day';
$handler->display->display_options['arguments']['date_argument']['date_fields'] = array(
  'field_data_field_datespan.field_datespan_value' => 'field_data_field_datespan.field_datespan_value',
);
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'ebms_event' => 'ebms_event',
);
$handler->display->display_options['filters']['type']['group'] = 1;
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = '1';
$handler->display->display_options['filters']['status']['group'] = 1;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
/* Filter criterion: Content: Event Category (field_event_category) */
$handler->display->display_options['filters']['field_event_category_value']['id'] = 'field_event_category_value';
$handler->display->display_options['filters']['field_event_category_value']['table'] = 'field_data_field_event_category';
$handler->display->display_options['filters']['field_event_category_value']['field'] = 'field_event_category_value';
$handler->display->display_options['filters']['field_event_category_value']['value'] = array(
  'board' => 'board',
);
$handler->display->display_options['path'] = 'calendar/day/all';
$handler->display->display_options['menu']['title'] = 'Day';
$handler->display->display_options['menu']['weight'] = '3';
$handler->display->display_options['menu']['context'] = 0;

/* Display: iCal */
$handler = $view->new_display('feed', 'iCal', 'ical_event_feed');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['pager']['type'] = 'some';
$handler->display->display_options['pager']['options']['items_per_page'] = '0';
$handler->display->display_options['pager']['options']['offset'] = '0';
$handler->display->display_options['style_plugin'] = 'date_ical';
$handler->display->display_options['style_options']['description'] = 'EBMS Event';
$handler->display->display_options['row_plugin'] = 'date_ical';
$handler->display->display_options['row_options']['date_field'] = 'field_data_field_datespan.field_datespan_value';
$handler->display->display_options['defaults']['relationships'] = FALSE;
/* Relationship: Content: Author */
$handler->display->display_options['relationships']['uid']['id'] = 'uid';
$handler->display->display_options['relationships']['uid']['table'] = 'node';
$handler->display->display_options['relationships']['uid']['field'] = 'uid';
$handler->display->display_options['relationships']['uid']['label'] = 'Submitter';
$handler->display->display_options['relationships']['uid']['required'] = TRUE;
$handler->display->display_options['defaults']['fields'] = FALSE;
/* Field: Content: Event Date */
$handler->display->display_options['fields']['field_datespan']['id'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['table'] = 'field_data_field_datespan';
$handler->display->display_options['fields']['field_datespan']['field'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['label'] = '';
$handler->display->display_options['fields']['field_datespan']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['hide_alter_empty'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['settings'] = array(
  'format_type' => 'short',
  'fromto' => 'both',
  'multiple_number' => '',
  'multiple_from' => '',
  'multiple_to' => '',
  'show_repeat_rule' => '',
);
$handler->display->display_options['fields']['field_datespan']['group_rows'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['delta_offset'] = '0';
/* Field: Content: Title */
$handler->display->display_options['fields']['title']['id'] = 'title';
$handler->display->display_options['fields']['title']['table'] = 'node';
$handler->display->display_options['fields']['title']['field'] = 'title';
$handler->display->display_options['fields']['title']['label'] = '';
$handler->display->display_options['fields']['title']['alter']['word_boundary'] = FALSE;
$handler->display->display_options['fields']['title']['alter']['ellipsis'] = FALSE;
$handler->display->display_options['fields']['title']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['title']['hide_alter_empty'] = FALSE;
/* Field: Field: Event Type */
$handler->display->display_options['fields']['field_event_type']['id'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type']['table'] = 'field_data_field_event_type';
$handler->display->display_options['fields']['field_event_type']['field'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type']['label'] = '';
$handler->display->display_options['fields']['field_event_type']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_event_type']['delta_offset'] = '0';
/* Field: Content: Author uid */
$handler->display->display_options['fields']['uid']['id'] = 'uid';
$handler->display->display_options['fields']['uid']['table'] = 'node';
$handler->display->display_options['fields']['uid']['field'] = 'uid';
$handler->display->display_options['fields']['uid']['label'] = 'Submitter';
/* Field: Field: Event Type */
$handler->display->display_options['fields']['field_event_type_1']['id'] = 'field_event_type_1';
$handler->display->display_options['fields']['field_event_type_1']['table'] = 'field_data_field_event_type';
$handler->display->display_options['fields']['field_event_type_1']['field'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type_1']['delta_offset'] = '0';
/* Field: Field: Notes */
$handler->display->display_options['fields']['field_notes']['id'] = 'field_notes';
$handler->display->display_options['fields']['field_notes']['table'] = 'field_data_field_notes';
$handler->display->display_options['fields']['field_notes']['field'] = 'field_notes';
/* Field: User: Name */
$handler->display->display_options['fields']['name']['id'] = 'name';
$handler->display->display_options['fields']['name']['table'] = 'users';
$handler->display->display_options['fields']['name']['field'] = 'name';
$handler->display->display_options['fields']['name']['relationship'] = 'uid';
$handler->display->display_options['fields']['name']['label'] = 'Submitter';
/* Field: Content: Event Status */
$handler->display->display_options['fields']['field_event_status']['id'] = 'field_event_status';
$handler->display->display_options['fields']['field_event_status']['table'] = 'field_data_field_event_status';
$handler->display->display_options['fields']['field_event_status']['field'] = 'field_event_status';
$handler->display->display_options['defaults']['sorts'] = FALSE;
$handler->display->display_options['defaults']['arguments'] = FALSE;
/* Contextual filter: Content: Nid */
$handler->display->display_options['arguments']['nid']['id'] = 'nid';
$handler->display->display_options['arguments']['nid']['table'] = 'node';
$handler->display->display_options['arguments']['nid']['field'] = 'nid';
$handler->display->display_options['arguments']['nid']['default_action'] = 'default';
$handler->display->display_options['arguments']['nid']['default_argument_type'] = 'node';
$handler->display->display_options['arguments']['nid']['summary']['number_of_records'] = '0';
$handler->display->display_options['arguments']['nid']['summary']['format'] = 'default_summary';
$handler->display->display_options['arguments']['nid']['summary_options']['items_per_page'] = '25';
$handler->display->display_options['arguments']['nid']['validate']['type'] = 'node';
$handler->display->display_options['arguments']['nid']['validate_options']['types'] = array(
  'ebms_event' => 'ebms_event',
);
$handler->display->display_options['path'] = 'node/%/event.ics';
$handler->display->display_options['displays'] = array(
  'default' => 0,
  'month' => 0,
  'week' => 0,
  'day' => 0,
);

/* Display: Mini-Calendar */
$handler = $view->new_display('block', 'Mini-Calendar', 'month_block');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'date_views_pager';
$handler->display->display_options['pager']['options']['date_id'] = 'mini';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'calendar_style';
$handler->display->display_options['style_options']['name_size'] = '1';
$handler->display->display_options['style_options']['mini'] = '1';
$handler->display->display_options['style_options']['with_weekno'] = '0';
$handler->display->display_options['style_options']['multiday_theme'] = '1';
$handler->display->display_options['style_options']['theme_style'] = '1';
$handler->display->display_options['style_options']['max_items'] = '0';
$handler->display->display_options['style_options']['groupby_times'] = 'half';
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'calendar_entity';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['arguments'] = FALSE;
/* Contextual filter: Date: Date (node) */
$handler->display->display_options['arguments']['date_argument']['id'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['table'] = 'node';
$handler->display->display_options['arguments']['date_argument']['field'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['default_action'] = 'default';
$handler->display->display_options['arguments']['date_argument']['default_argument_type'] = 'date';
$handler->display->display_options['arguments']['date_argument']['summary']['format'] = 'default_summary';
$handler->display->display_options['arguments']['date_argument']['add_delta'] = 'yes';
$handler->display->display_options['arguments']['date_argument']['date_fields'] = array(
  'field_data_field_datespan.field_datespan_value' => 'field_data_field_datespan.field_datespan_value',
);
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'ebms_event' => 'ebms_event',
);
$handler->display->display_options['filters']['type']['group'] = 1;
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = '1';
$handler->display->display_options['filters']['status']['group'] = 1;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
/* Filter criterion: Content: Event Status (field_event_status) */
$handler->display->display_options['filters']['field_event_status_value']['id'] = 'field_event_status_value';
$handler->display->display_options['filters']['field_event_status_value']['table'] = 'field_data_field_event_status';
$handler->display->display_options['filters']['field_event_status_value']['field'] = 'field_event_status_value';
$handler->display->display_options['filters']['field_event_status_value']['value'] = array(
  'scheduled' => 'scheduled',
);

/* Display: Upcoming Events */
$handler = $view->new_display('block', 'Upcoming Events', 'block_upcoming');
$handler->display->display_options['defaults']['title'] = FALSE;
$handler->display->display_options['title'] = 'Upcoming Events';
$handler->display->display_options['display_description'] = 'Show up to six upcoming events.';
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'some';
$handler->display->display_options['pager']['options']['items_per_page'] = '6';
$handler->display->display_options['pager']['options']['offset'] = '0';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'list';
$handler->display->display_options['style_options']['grouping'] = array(
  0 => array(
    'field' => 'field_datespan_1',
    'rendered' => 1,
    'rendered_strip' => 1,
  ),
);
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'fields';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['header'] = FALSE;
/* Header: Global: Text area */
$handler->display->display_options['header']['area']['id'] = 'area';
$handler->display->display_options['header']['area']['table'] = 'views';
$handler->display->display_options['header']['area']['field'] = 'area';
$handler->display->display_options['header']['area']['label'] = 'Title';
$handler->display->display_options['header']['area']['empty'] = TRUE;
$handler->display->display_options['header']['area']['content'] = '<h3>Upcoming Events</h3>';
$handler->display->display_options['header']['area']['format'] = 'full_html';
$handler->display->display_options['defaults']['footer'] = FALSE;
/* Footer: Global: Text area */
$handler->display->display_options['footer']['area']['id'] = 'area';
$handler->display->display_options['footer']['area']['table'] = 'views';
$handler->display->display_options['footer']['area']['field'] = 'area';
$handler->display->display_options['footer']['area']['empty'] = TRUE;
$handler->display->display_options['footer']['area']['content'] = '<div></div>';
$handler->display->display_options['footer']['area']['format'] = 'full_html';
$handler->display->display_options['defaults']['empty'] = FALSE;
/* No results behavior: Global: Text area */
$handler->display->display_options['empty']['area']['id'] = 'area';
$handler->display->display_options['empty']['area']['table'] = 'views';
$handler->display->display_options['empty']['area']['field'] = 'area';
$handler->display->display_options['empty']['area']['empty'] = TRUE;
$handler->display->display_options['empty']['area']['content'] = '<em>You have no upcoming scheduled events.</em>';
$handler->display->display_options['empty']['area']['format'] = 'filtered_html';
$handler->display->display_options['defaults']['fields'] = FALSE;
/* Field: Event Time */
$handler->display->display_options['fields']['field_datespan']['id'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['table'] = 'field_data_field_datespan';
$handler->display->display_options['fields']['field_datespan']['field'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['ui_name'] = 'Event Time';
$handler->display->display_options['fields']['field_datespan']['label'] = '';
$handler->display->display_options['fields']['field_datespan']['alter']['alter_text'] = TRUE;
$handler->display->display_options['fields']['field_datespan']['alter']['text'] = '[field_datespan] E.T.';
$handler->display->display_options['fields']['field_datespan']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['hide_alter_empty'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['settings'] = array(
  'format_type' => 'ebms_time',
  'fromto' => 'both',
  'multiple_number' => '',
  'multiple_from' => '',
  'multiple_to' => '',
);
$handler->display->display_options['fields']['field_datespan']['group_rows'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['delta_offset'] = '0';
/* Field: Content: Title */
$handler->display->display_options['fields']['title']['id'] = 'title';
$handler->display->display_options['fields']['title']['table'] = 'node';
$handler->display->display_options['fields']['title']['field'] = 'title';
$handler->display->display_options['fields']['title']['label'] = '';
$handler->display->display_options['fields']['title']['alter']['word_boundary'] = FALSE;
$handler->display->display_options['fields']['title']['alter']['ellipsis'] = FALSE;
$handler->display->display_options['fields']['title']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['title']['hide_alter_empty'] = FALSE;
/* Field: Field: Event Type */
$handler->display->display_options['fields']['field_event_type']['id'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type']['table'] = 'field_data_field_event_type';
$handler->display->display_options['fields']['field_event_type']['field'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type']['label'] = '';
$handler->display->display_options['fields']['field_event_type']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_event_type']['delta_offset'] = '0';
/* Field: Event Day */
$handler->display->display_options['fields']['field_datespan_1']['id'] = 'field_datespan_1';
$handler->display->display_options['fields']['field_datespan_1']['table'] = 'field_data_field_datespan';
$handler->display->display_options['fields']['field_datespan_1']['field'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan_1']['ui_name'] = 'Event Day';
$handler->display->display_options['fields']['field_datespan_1']['label'] = '';
$handler->display->display_options['fields']['field_datespan_1']['exclude'] = TRUE;
$handler->display->display_options['fields']['field_datespan_1']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_datespan_1']['settings'] = array(
  'format_type' => 'ebms_date',
  'fromto' => 'value',
  'multiple_number' => '',
  'multiple_from' => '',
  'multiple_to' => '',
);
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = 1;
$handler->display->display_options['filters']['status']['group'] = 0;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'ebms_event' => 'ebms_event',
);
/* Filter criterion: Content: Event Date -  start date (field_datespan) */
$handler->display->display_options['filters']['field_datespan_value']['id'] = 'field_datespan_value';
$handler->display->display_options['filters']['field_datespan_value']['table'] = 'field_data_field_datespan';
$handler->display->display_options['filters']['field_datespan_value']['field'] = 'field_datespan_value';
$handler->display->display_options['filters']['field_datespan_value']['operator'] = '>=';
$handler->display->display_options['filters']['field_datespan_value']['default_date'] = 'now';
/* Filter criterion: Content: Event Status (field_event_status) */
$handler->display->display_options['filters']['field_event_status_value']['id'] = 'field_event_status_value';
$handler->display->display_options['filters']['field_event_status_value']['table'] = 'field_data_field_event_status';
$handler->display->display_options['filters']['field_event_status_value']['field'] = 'field_event_status_value';
$handler->display->display_options['filters']['field_event_status_value']['value'] = array(
  'scheduled' => 'scheduled',
);
$handler->display->display_options['block_description'] = 'Upcoming Events';

/* Display: Next Event */
$handler = $view->new_display('block', 'Next Event', 'block_next');
$handler->display->display_options['display_description'] = 'Show up to six upcoming events.';
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'some';
$handler->display->display_options['pager']['options']['items_per_page'] = '1';
$handler->display->display_options['pager']['options']['offset'] = '0';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'list';
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'fields';
$handler->display->display_options['row_options']['inline'] = array(
  'title' => 'title',
  'field_event_type' => 'field_event_type',
);
$handler->display->display_options['row_options']['separator'] = '&ndash;';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['empty'] = FALSE;
/* No results behavior: Global: Text area */
$handler->display->display_options['empty']['area']['id'] = 'area';
$handler->display->display_options['empty']['area']['table'] = 'views';
$handler->display->display_options['empty']['area']['field'] = 'area';
$handler->display->display_options['empty']['area']['empty'] = TRUE;
$handler->display->display_options['empty']['area']['content'] = '<em>You have no upcoming scheduled events.</em>';
$handler->display->display_options['empty']['area']['format'] = 'filtered_html';
$handler->display->display_options['defaults']['fields'] = FALSE;
/* Field: Event Day */
$handler->display->display_options['fields']['field_datespan_1']['id'] = 'field_datespan_1';
$handler->display->display_options['fields']['field_datespan_1']['table'] = 'field_data_field_datespan';
$handler->display->display_options['fields']['field_datespan_1']['field'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan_1']['ui_name'] = 'Event Day';
$handler->display->display_options['fields']['field_datespan_1']['label'] = '';
$handler->display->display_options['fields']['field_datespan_1']['alter']['text'] = '<h3>[field_datespan_1]';
$handler->display->display_options['fields']['field_datespan_1']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_datespan_1']['settings'] = array(
  'format_type' => 'ebms_date',
  'fromto' => 'both',
  'multiple_number' => '',
  'multiple_from' => '',
  'multiple_to' => '',
);
/* Field: Content: Event Date */
$handler->display->display_options['fields']['field_datespan']['id'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['table'] = 'field_data_field_datespan';
$handler->display->display_options['fields']['field_datespan']['field'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['label'] = '';
$handler->display->display_options['fields']['field_datespan']['alter']['alter_text'] = TRUE;
$handler->display->display_options['fields']['field_datespan']['alter']['text'] = '[field_datespan] E.T.';
$handler->display->display_options['fields']['field_datespan']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['hide_alter_empty'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['settings'] = array(
  'format_type' => 'ebms_time',
  'fromto' => 'both',
  'multiple_number' => '',
  'multiple_from' => '',
  'multiple_to' => '',
);
$handler->display->display_options['fields']['field_datespan']['group_rows'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['delta_offset'] = '0';
/* Field: Content: Title */
$handler->display->display_options['fields']['title']['id'] = 'title';
$handler->display->display_options['fields']['title']['table'] = 'node';
$handler->display->display_options['fields']['title']['field'] = 'title';
$handler->display->display_options['fields']['title']['label'] = '';
$handler->display->display_options['fields']['title']['alter']['word_boundary'] = FALSE;
$handler->display->display_options['fields']['title']['alter']['ellipsis'] = FALSE;
$handler->display->display_options['fields']['title']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['title']['hide_alter_empty'] = FALSE;
/* Field: Field: Event Type */
$handler->display->display_options['fields']['field_event_type']['id'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type']['table'] = 'field_data_field_event_type';
$handler->display->display_options['fields']['field_event_type']['field'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type']['label'] = '';
$handler->display->display_options['fields']['field_event_type']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_event_type']['delta_offset'] = '0';
/* Field: Content: Agenda */
$handler->display->display_options['fields']['field_agenda']['id'] = 'field_agenda';
$handler->display->display_options['fields']['field_agenda']['table'] = 'field_data_field_agenda';
$handler->display->display_options['fields']['field_agenda']['field'] = 'field_agenda';
$handler->display->display_options['fields']['field_agenda']['label'] = '';
$handler->display->display_options['fields']['field_agenda']['alter']['alter_text'] = TRUE;
$handler->display->display_options['fields']['field_agenda']['alter']['text'] = ' (Agenda Posted)';
$handler->display->display_options['fields']['field_agenda']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_agenda']['hide_empty'] = TRUE;
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = 1;
$handler->display->display_options['filters']['status']['group'] = 0;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'ebms_event' => 'ebms_event',
);
/* Filter criterion: Content: Event Date -  start date (field_datespan) */
$handler->display->display_options['filters']['field_datespan_value']['id'] = 'field_datespan_value';
$handler->display->display_options['filters']['field_datespan_value']['table'] = 'field_data_field_datespan';
$handler->display->display_options['filters']['field_datespan_value']['field'] = 'field_datespan_value';
$handler->display->display_options['filters']['field_datespan_value']['operator'] = '>=';
$handler->display->display_options['filters']['field_datespan_value']['default_date'] = 'now';
/* Filter criterion: Content: Event Status (field_event_status) */
$handler->display->display_options['filters']['field_event_status_value']['id'] = 'field_event_status_value';
$handler->display->display_options['filters']['field_event_status_value']['table'] = 'field_data_field_event_status';
$handler->display->display_options['filters']['field_event_status_value']['field'] = 'field_event_status_value';
$handler->display->display_options['filters']['field_event_status_value']['value'] = array(
  'scheduled' => 'scheduled',
);

/* Display: Month */
$handler = $view->new_display('page', 'Month', 'month');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'date_views_pager';
$handler->display->display_options['pager']['options']['date_id'] = 'month';
$handler->display->display_options['pager']['options']['link_format'] = 'clean';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'calendar_style';
$handler->display->display_options['style_options']['name_size'] = '3';
$handler->display->display_options['style_options']['mini'] = '0';
$handler->display->display_options['style_options']['with_weekno'] = '0';
$handler->display->display_options['style_options']['multiday_theme'] = '1';
$handler->display->display_options['style_options']['theme_style'] = '1';
$handler->display->display_options['style_options']['max_items'] = '0';
$handler->display->display_options['style_options']['groupby_times'] = 'half';
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'calendar_entity';
$handler->display->display_options['row_options']['colors']['legend'] = 'type';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['arguments'] = FALSE;
/* Contextual filter: Date: Date (node) */
$handler->display->display_options['arguments']['date_argument']['id'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['table'] = 'node';
$handler->display->display_options['arguments']['date_argument']['field'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['default_action'] = 'default';
$handler->display->display_options['arguments']['date_argument']['default_argument_type'] = 'date';
$handler->display->display_options['arguments']['date_argument']['summary']['format'] = 'default_summary';
$handler->display->display_options['arguments']['date_argument']['add_delta'] = 'yes';
$handler->display->display_options['arguments']['date_argument']['date_fields'] = array(
  'field_data_field_datespan.field_datespan_value' => 'field_data_field_datespan.field_datespan_value',
);
$handler->display->display_options['path'] = 'calendar';
$handler->display->display_options['menu']['type'] = 'default tab';
$handler->display->display_options['menu']['title'] = 'Month';
$handler->display->display_options['menu']['weight'] = '0';
$handler->display->display_options['menu']['context'] = 0;
$handler->display->display_options['tab_options']['type'] = 'normal';
$handler->display->display_options['tab_options']['title'] = 'Calendar';
$handler->display->display_options['tab_options']['weight'] = '0';

/* Display: Week */
$handler = $view->new_display('page', 'Week', 'week');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'date_views_pager';
$handler->display->display_options['pager']['options']['date_id'] = 'week';
$handler->display->display_options['pager']['options']['link_format'] = 'clean';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'calendar_style';
$handler->display->display_options['style_options']['calendar_type'] = 'week';
$handler->display->display_options['style_options']['name_size'] = '3';
$handler->display->display_options['style_options']['mini'] = '0';
$handler->display->display_options['style_options']['with_weekno'] = '0';
$handler->display->display_options['style_options']['multiday_theme'] = '1';
$handler->display->display_options['style_options']['theme_style'] = '1';
$handler->display->display_options['style_options']['max_items'] = '0';
$handler->display->display_options['style_options']['groupby_times'] = 'half';
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'calendar_entity';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['arguments'] = FALSE;
/* Contextual filter: Date: Date (node) */
$handler->display->display_options['arguments']['date_argument']['id'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['table'] = 'node';
$handler->display->display_options['arguments']['date_argument']['field'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['default_action'] = 'default';
$handler->display->display_options['arguments']['date_argument']['default_argument_type'] = 'date';
$handler->display->display_options['arguments']['date_argument']['summary']['format'] = 'default_summary';
$handler->display->display_options['arguments']['date_argument']['granularity'] = 'week';
$handler->display->display_options['arguments']['date_argument']['add_delta'] = 'yes';
$handler->display->display_options['arguments']['date_argument']['date_fields'] = array(
  'field_data_field_datespan.field_datespan_value' => 'field_data_field_datespan.field_datespan_value',
);
$handler->display->display_options['path'] = 'calendar/week';
$handler->display->display_options['menu']['type'] = 'tab';
$handler->display->display_options['menu']['title'] = 'Week';
$handler->display->display_options['menu']['weight'] = '2';
$handler->display->display_options['menu']['context'] = 0;

/* Display: Day */
$handler = $view->new_display('page', 'Day', 'day');
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'date_views_pager';
$handler->display->display_options['pager']['options']['date_id'] = 'day';
$handler->display->display_options['pager']['options']['link_format'] = 'clean';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'calendar_style';
$handler->display->display_options['style_options']['calendar_type'] = 'day';
$handler->display->display_options['style_options']['name_size'] = '3';
$handler->display->display_options['style_options']['mini'] = '0';
$handler->display->display_options['style_options']['with_weekno'] = '0';
$handler->display->display_options['style_options']['multiday_theme'] = '1';
$handler->display->display_options['style_options']['theme_style'] = '1';
$handler->display->display_options['style_options']['max_items'] = '0';
$handler->display->display_options['style_options']['groupby_times'] = 'half';
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'calendar_entity';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['empty'] = FALSE;
$handler->display->display_options['defaults']['arguments'] = FALSE;
/* Contextual filter: Date: Date (node) */
$handler->display->display_options['arguments']['date_argument']['id'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['table'] = 'node';
$handler->display->display_options['arguments']['date_argument']['field'] = 'date_argument';
$handler->display->display_options['arguments']['date_argument']['default_action'] = 'default';
$handler->display->display_options['arguments']['date_argument']['default_argument_type'] = 'date';
$handler->display->display_options['arguments']['date_argument']['summary']['format'] = 'default_summary';
$handler->display->display_options['arguments']['date_argument']['granularity'] = 'day';
$handler->display->display_options['arguments']['date_argument']['date_fields'] = array(
  'field_data_field_datespan.field_datespan_value' => 'field_data_field_datespan.field_datespan_value',
);
$handler->display->display_options['path'] = 'calendar/day';
$handler->display->display_options['menu']['type'] = 'tab';
$handler->display->display_options['menu']['title'] = 'Day';
$handler->display->display_options['menu']['weight'] = '3';
$handler->display->display_options['menu']['context'] = 0;

/* Display: Past Events */
$handler = $view->new_display('block', 'Past Events', 'block_past');
$handler->display->display_options['defaults']['title'] = FALSE;
$handler->display->display_options['title'] = 'Past Events';
$handler->display->display_options['display_description'] = 'Display all past events.';
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'none';
$handler->display->display_options['pager']['options']['offset'] = '0';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'list';
$handler->display->display_options['style_options']['grouping'] = array(
  0 => array(
    'field' => 'field_datespan_1',
    'rendered' => 1,
    'rendered_strip' => 1,
  ),
);
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'fields';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['header'] = FALSE;
/* Header: Global: Text area */
$handler->display->display_options['header']['area']['id'] = 'area';
$handler->display->display_options['header']['area']['table'] = 'views';
$handler->display->display_options['header']['area']['field'] = 'area';
$handler->display->display_options['header']['area']['label'] = 'Title';
$handler->display->display_options['header']['area']['empty'] = TRUE;
$handler->display->display_options['header']['area']['content'] = '<h3>Upcoming Events</h3>';
$handler->display->display_options['header']['area']['format'] = 'full_html';
$handler->display->display_options['defaults']['footer'] = FALSE;
/* Footer: Global: Text area */
$handler->display->display_options['footer']['area']['id'] = 'area';
$handler->display->display_options['footer']['area']['table'] = 'views';
$handler->display->display_options['footer']['area']['field'] = 'area';
$handler->display->display_options['footer']['area']['empty'] = TRUE;
$handler->display->display_options['footer']['area']['content'] = '<div></div>';
$handler->display->display_options['footer']['area']['format'] = 'full_html';
$handler->display->display_options['defaults']['empty'] = FALSE;
/* No results behavior: Global: Text area */
$handler->display->display_options['empty']['area']['id'] = 'area';
$handler->display->display_options['empty']['area']['table'] = 'views';
$handler->display->display_options['empty']['area']['field'] = 'area';
$handler->display->display_options['empty']['area']['empty'] = TRUE;
$handler->display->display_options['empty']['area']['content'] = '<em>You have no upcoming scheduled events.</em>';
$handler->display->display_options['empty']['area']['format'] = 'filtered_html';
$handler->display->display_options['defaults']['fields'] = FALSE;
/* Field: Event Time */
$handler->display->display_options['fields']['field_datespan']['id'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['table'] = 'field_data_field_datespan';
$handler->display->display_options['fields']['field_datespan']['field'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['ui_name'] = 'Event Time';
$handler->display->display_options['fields']['field_datespan']['label'] = '';
$handler->display->display_options['fields']['field_datespan']['alter']['alter_text'] = TRUE;
$handler->display->display_options['fields']['field_datespan']['alter']['text'] = '[field_datespan] E.T.';
$handler->display->display_options['fields']['field_datespan']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['hide_alter_empty'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['settings'] = array(
  'format_type' => 'ebms_time',
  'fromto' => 'both',
  'multiple_number' => '',
  'multiple_from' => '',
  'multiple_to' => '',
);
$handler->display->display_options['fields']['field_datespan']['group_rows'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['delta_offset'] = '0';
/* Field: Content: Title */
$handler->display->display_options['fields']['title']['id'] = 'title';
$handler->display->display_options['fields']['title']['table'] = 'node';
$handler->display->display_options['fields']['title']['field'] = 'title';
$handler->display->display_options['fields']['title']['label'] = '';
$handler->display->display_options['fields']['title']['alter']['word_boundary'] = FALSE;
$handler->display->display_options['fields']['title']['alter']['ellipsis'] = FALSE;
$handler->display->display_options['fields']['title']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['title']['hide_alter_empty'] = FALSE;
/* Field: Field: Event Type */
$handler->display->display_options['fields']['field_event_type']['id'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type']['table'] = 'field_data_field_event_type';
$handler->display->display_options['fields']['field_event_type']['field'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type']['label'] = '';
$handler->display->display_options['fields']['field_event_type']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_event_type']['delta_offset'] = '0';
/* Field: Event Day */
$handler->display->display_options['fields']['field_datespan_1']['id'] = 'field_datespan_1';
$handler->display->display_options['fields']['field_datespan_1']['table'] = 'field_data_field_datespan';
$handler->display->display_options['fields']['field_datespan_1']['field'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan_1']['ui_name'] = 'Event Day';
$handler->display->display_options['fields']['field_datespan_1']['label'] = '';
$handler->display->display_options['fields']['field_datespan_1']['exclude'] = TRUE;
$handler->display->display_options['fields']['field_datespan_1']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_datespan_1']['settings'] = array(
  'format_type' => 'ebms_date',
  'fromto' => 'value',
  'multiple_number' => '',
  'multiple_from' => '',
  'multiple_to' => '',
);
$handler->display->display_options['defaults']['sorts'] = FALSE;
/* Sort criterion: Content: Event Date -  start date (field_datespan) */
$handler->display->display_options['sorts']['field_datespan_value']['id'] = 'field_datespan_value';
$handler->display->display_options['sorts']['field_datespan_value']['table'] = 'field_data_field_datespan';
$handler->display->display_options['sorts']['field_datespan_value']['field'] = 'field_datespan_value';
$handler->display->display_options['sorts']['field_datespan_value']['order'] = 'DESC';
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = 1;
$handler->display->display_options['filters']['status']['group'] = 0;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'ebms_event' => 'ebms_event',
);
/* Filter criterion: field_datespan is not in the future */
$handler->display->display_options['filters']['field_datespan_value2']['id'] = 'field_datespan_value2';
$handler->display->display_options['filters']['field_datespan_value2']['table'] = 'field_data_field_datespan';
$handler->display->display_options['filters']['field_datespan_value2']['field'] = 'field_datespan_value2';
$handler->display->display_options['filters']['field_datespan_value2']['ui_name'] = 'field_datespan is not in the future';
$handler->display->display_options['filters']['field_datespan_value2']['operator'] = '<';
$handler->display->display_options['filters']['field_datespan_value2']['granularity'] = 'second';
$handler->display->display_options['filters']['field_datespan_value2']['default_date'] = 'tomorrow';
/* Filter criterion: Content: Event Status (field_event_status) */
$handler->display->display_options['filters']['field_event_status_value']['id'] = 'field_event_status_value';
$handler->display->display_options['filters']['field_event_status_value']['table'] = 'field_data_field_event_status';
$handler->display->display_options['filters']['field_event_status_value']['field'] = 'field_event_status_value';
$handler->display->display_options['filters']['field_event_status_value']['value'] = array(
  'scheduled' => 'scheduled',
);
$handler->display->display_options['block_description'] = 'Past Events';

/* Display: Upcoming In-Person Events */
$handler = $view->new_display('block', 'Upcoming In-Person Events', 'block_inperson_upcoming');
$handler->display->display_options['defaults']['title'] = FALSE;
$handler->display->display_options['title'] = 'Upcoming In-Person Events';
$handler->display->display_options['display_description'] = 'Show up to six upcoming in-person events.';
$handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
$handler->display->display_options['defaults']['access'] = FALSE;
$handler->display->display_options['access']['type'] = 'perm';
$handler->display->display_options['defaults']['pager'] = FALSE;
$handler->display->display_options['pager']['type'] = 'some';
$handler->display->display_options['pager']['options']['items_per_page'] = '6';
$handler->display->display_options['pager']['options']['offset'] = '0';
$handler->display->display_options['defaults']['style_plugin'] = FALSE;
$handler->display->display_options['style_plugin'] = 'list';
$handler->display->display_options['style_options']['grouping'] = array(
  0 => array(
    'field' => 'field_datespan_1',
    'rendered' => 1,
    'rendered_strip' => 1,
  ),
);
$handler->display->display_options['defaults']['style_options'] = FALSE;
$handler->display->display_options['defaults']['row_plugin'] = FALSE;
$handler->display->display_options['row_plugin'] = 'fields';
$handler->display->display_options['defaults']['row_options'] = FALSE;
$handler->display->display_options['defaults']['header'] = FALSE;
/* Header: Global: Text area */
$handler->display->display_options['header']['area']['id'] = 'area';
$handler->display->display_options['header']['area']['table'] = 'views';
$handler->display->display_options['header']['area']['field'] = 'area';
$handler->display->display_options['header']['area']['label'] = 'Title';
$handler->display->display_options['header']['area']['empty'] = TRUE;
$handler->display->display_options['header']['area']['content'] = '<h3>Upcoming In-Person Events</h3>';
$handler->display->display_options['header']['area']['format'] = 'full_html';
$handler->display->display_options['defaults']['footer'] = FALSE;
/* Footer: Global: Text area */
$handler->display->display_options['footer']['area']['id'] = 'area';
$handler->display->display_options['footer']['area']['table'] = 'views';
$handler->display->display_options['footer']['area']['field'] = 'area';
$handler->display->display_options['footer']['area']['empty'] = TRUE;
$handler->display->display_options['footer']['area']['content'] = '<div></div>';
$handler->display->display_options['footer']['area']['format'] = 'full_html';
$handler->display->display_options['defaults']['empty'] = FALSE;
/* No results behavior: Global: Text area */
$handler->display->display_options['empty']['area']['id'] = 'area';
$handler->display->display_options['empty']['area']['table'] = 'views';
$handler->display->display_options['empty']['area']['field'] = 'area';
$handler->display->display_options['empty']['area']['empty'] = TRUE;
$handler->display->display_options['empty']['area']['content'] = '<em>You have no upcoming scheduled events.</em>';
$handler->display->display_options['empty']['area']['format'] = 'filtered_html';
$handler->display->display_options['defaults']['fields'] = FALSE;
/* Field: Event Time */
$handler->display->display_options['fields']['field_datespan']['id'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['table'] = 'field_data_field_datespan';
$handler->display->display_options['fields']['field_datespan']['field'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan']['ui_name'] = 'Event Time';
$handler->display->display_options['fields']['field_datespan']['label'] = '';
$handler->display->display_options['fields']['field_datespan']['alter']['alter_text'] = TRUE;
$handler->display->display_options['fields']['field_datespan']['alter']['text'] = '[field_datespan] E.T.';
$handler->display->display_options['fields']['field_datespan']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['hide_alter_empty'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['settings'] = array(
  'format_type' => 'ebms_time',
  'fromto' => 'both',
  'multiple_number' => '',
  'multiple_from' => '',
  'multiple_to' => '',
);
$handler->display->display_options['fields']['field_datespan']['group_rows'] = FALSE;
$handler->display->display_options['fields']['field_datespan']['delta_offset'] = '0';
/* Field: Content: Title */
$handler->display->display_options['fields']['title']['id'] = 'title';
$handler->display->display_options['fields']['title']['table'] = 'node';
$handler->display->display_options['fields']['title']['field'] = 'title';
$handler->display->display_options['fields']['title']['label'] = '';
$handler->display->display_options['fields']['title']['alter']['word_boundary'] = FALSE;
$handler->display->display_options['fields']['title']['alter']['ellipsis'] = FALSE;
$handler->display->display_options['fields']['title']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['title']['hide_alter_empty'] = FALSE;
/* Field: Field: Event Type */
$handler->display->display_options['fields']['field_event_type']['id'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type']['table'] = 'field_data_field_event_type';
$handler->display->display_options['fields']['field_event_type']['field'] = 'field_event_type';
$handler->display->display_options['fields']['field_event_type']['label'] = '';
$handler->display->display_options['fields']['field_event_type']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_event_type']['delta_offset'] = '0';
/* Field: Event Day */
$handler->display->display_options['fields']['field_datespan_1']['id'] = 'field_datespan_1';
$handler->display->display_options['fields']['field_datespan_1']['table'] = 'field_data_field_datespan';
$handler->display->display_options['fields']['field_datespan_1']['field'] = 'field_datespan';
$handler->display->display_options['fields']['field_datespan_1']['ui_name'] = 'Event Day';
$handler->display->display_options['fields']['field_datespan_1']['label'] = '';
$handler->display->display_options['fields']['field_datespan_1']['exclude'] = TRUE;
$handler->display->display_options['fields']['field_datespan_1']['element_label_colon'] = FALSE;
$handler->display->display_options['fields']['field_datespan_1']['settings'] = array(
  'format_type' => 'ebms_date',
  'fromto' => 'value',
  'multiple_number' => '',
  'multiple_from' => '',
  'multiple_to' => '',
);
$handler->display->display_options['defaults']['filter_groups'] = FALSE;
$handler->display->display_options['defaults']['filters'] = FALSE;
/* Filter criterion: Content: Published */
$handler->display->display_options['filters']['status']['id'] = 'status';
$handler->display->display_options['filters']['status']['table'] = 'node';
$handler->display->display_options['filters']['status']['field'] = 'status';
$handler->display->display_options['filters']['status']['value'] = 1;
$handler->display->display_options['filters']['status']['group'] = 0;
$handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
/* Filter criterion: Content: Type */
$handler->display->display_options['filters']['type']['id'] = 'type';
$handler->display->display_options['filters']['type']['table'] = 'node';
$handler->display->display_options['filters']['type']['field'] = 'type';
$handler->display->display_options['filters']['type']['value'] = array(
  'ebms_event' => 'ebms_event',
);
/* Filter criterion: Content: Event Date -  start date (field_datespan) */
$handler->display->display_options['filters']['field_datespan_value']['id'] = 'field_datespan_value';
$handler->display->display_options['filters']['field_datespan_value']['table'] = 'field_data_field_datespan';
$handler->display->display_options['filters']['field_datespan_value']['field'] = 'field_datespan_value';
$handler->display->display_options['filters']['field_datespan_value']['operator'] = '>=';
$handler->display->display_options['filters']['field_datespan_value']['default_date'] = 'now';
/* Filter criterion: Content: Event Status (field_event_status) */
$handler->display->display_options['filters']['field_event_status_value']['id'] = 'field_event_status_value';
$handler->display->display_options['filters']['field_event_status_value']['table'] = 'field_data_field_event_status';
$handler->display->display_options['filters']['field_event_status_value']['field'] = 'field_event_status_value';
$handler->display->display_options['filters']['field_event_status_value']['value'] = array(
  'scheduled' => 'scheduled',
);
/* Filter criterion: Field: Event Type (field_event_type) */
$handler->display->display_options['filters']['field_event_type_value']['id'] = 'field_event_type_value';
$handler->display->display_options['filters']['field_event_type_value']['table'] = 'field_data_field_event_type';
$handler->display->display_options['filters']['field_event_type_value']['field'] = 'field_event_type_value';
$handler->display->display_options['filters']['field_event_type_value']['value'] = array(
  'in_person' => 'in_person',
);
$handler->display->display_options['block_description'] = 'Upcoming In-Person Events';
