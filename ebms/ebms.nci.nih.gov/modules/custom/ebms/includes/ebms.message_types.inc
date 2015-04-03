<?php

// Use https://ebms.nci.nih.gov/dump-message-types to generate current definitions.
$pdq_ebms_recent_activity_message_types = array(
  'activity_new_articles' => '{
    "name" : "activity_new_articles",
    "description" : "New Articles",
    "argument_keys" : [],
    "argument" : [],
    "category" : "message_type",
    "data" : { "purge" : { "override" : 0, "enabled" : 0, "quota" : "", "days" : "" } },
    "language" : "",
    "arguments" : null,
    "message_text" : { "und" : [
        {
          "value" : "New [message:field_boards] articles posted.  [message:timestamp:custom:m\/d\/Y]",
          "format" : "plain_text",
          "safe_value" : "\u003Cp\u003ENew [message:field_boards] articles posted.  [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
        }
      ]
    }
  }',
  'activity_new_event' => '{
    "name" : "activity_new_event",
    "description" : "New Event",
    "argument_keys" : [],
    "argument" : [],
    "category" : "message_type",
    "data" : { "purge" : { "override" : 0, "enabled" : 0, "quota" : "", "days" : "" } },
    "language" : "",
    "arguments" : null,
    "message_text" : { "und" : [
        {
          "value" : "New event [message:field_title] posted.  [message:timestamp:custom:m\/d\/Y]",
          "format" : "plain_text",
          "safe_value" : "\u003Cp\u003ENew event [message:field_title] posted.  [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
        }
      ]
    }
  }',
  'activity_changed_event' => '{
    "name" : "activity_changed_event",
    "description" : "Changed Event",
    "argument_keys" : [],
    "argument" : [],
    "category" : "message_type",
    "data" : { "purge" : { "override" : 0, "enabled" : 0, "quota" : "", "days" : "" } },
    "language" : "",
    "arguments" : null,
    "message_text" : { "und" : [
        {
          "value" : "[message:field_title] time updated.  [message:timestamp:custom:m\/d\/Y]",
          "format" : "plain_text",
          "safe_value" : "\u003Cp\u003E[message:field_title] time updated.  [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
        }
      ]
    }
  }',
  'activity_cancelled_event' => '{
    "name" : "activity_cancelled_event",
    "description" : "Cancelled Event",
    "argument_keys" : [],
    "argument" : [],
    "category" : "message_type",
    "data" : { "purge" : { "override" : 0, "enabled" : 0, "quota" : "", "days" : "" } },
    "language" : "",
    "arguments" : null,
    "message_text" : { "und" : [
        {
          "value" : "Event [message:field_title] cancelled.  [message:timestamp:custom:m\/d\/Y]",
          "format" : "plain_text",
          "safe_value" : "\u003Cp\u003EEvent [message:field_title] cancelled.  [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
        }
      ]
    }
  }',
  'activity_agenda_published' => '{
    "name" : "activity_agenda_published",
    "description" : "Agenda Published",
    "argument_keys" : [],
    "argument" : [],
    "category" : "message_type",
    "data" : { "purge" : { "override" : 0, "enabled" : 0, "quota" : "", "days" : "" } },
    "language" : "",
    "arguments" : null,
    "message_text" : { "und" : [
        {
          "value" : "Agenda published for [message:field_title]. [message:timestamp:custom:m\/d\/Y]",
          "format" : "plain_text",
          "safe_value" : "\u003Cp\u003EAgenda published for [message:field_title]. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
        }
      ]
    }
  }',
  'activity_event_type' => '{
    "name" : "activity_event_type",
    "description" : "Event Type Changed",
    "argument_keys" : [],
    "argument" : [],
    "category" : "message_type",
    "data" : { "purge" : { "override" : 0, "enabled" : 0, "quota" : "", "days" : "" } },
    "language" : "",
    "arguments" : null,
    "message_text" : { "und" : [
        {
          "value" : "[message:field_title] changed to [message:field_event_type].  [message:timestamp:custom:m\/d\/Y]",
          "format" : "plain_text",
          "safe_value" : "\u003Cp\u003E[message:field_title] changed to [message:field_event_type].  [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
        }
      ]
    }
  }',
  'activity_new_packet' => '{
    "name" : "activity_new_packet",
    "description" : "New Packet",
    "argument_keys" : [],
    "argument" : [],
    "category" : "message_type",
    "data" : { "purge" : { "override" : 0, "enabled" : 0, "quota" : "", "days" : "" } },
    "language" : "",
    "arguments" : null,
    "message_text" : { "und" : [
        {
          "value" : "[message:field_title] literature posted.  [message:timestamp:custom:m\/d\/Y]",
          "format" : "plain_text",
          "safe_value" : "\u003Cp\u003E[message:field_title] literature posted.  [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
        }
      ]
    }
  }',
  'activity_new_summary' => '{
    "name" : "activity_new_summary",
    "description" : "New Summary",
    "argument_keys" : [],
    "argument" : [],
    "category" : "message_type",
    "data" : { "purge" : { "override" : 0, "enabled" : 0, "quota" : "", "days" : "" } },
    "language" : "",
    "arguments" : null,
    "message_text" : { "und" : [
        {
          "value" : "[message:user:name] posted [message:field_title].  [message:timestamp:custom:m\/d\/Y]",
          "format" : "plain_text",
          "safe_value" : "\u003Cp\u003E[message:user:name] posted [message:field_title].  [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
        }
      ]
    }
  }',
);
