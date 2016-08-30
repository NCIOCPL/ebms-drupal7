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
          "value" : "\u003Cp\u003E\u003Ca href=\u0022\/citations\u0022\u003ENew [message:field_boards] articles posted.\u003C\/a\u003E  [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E",
          "format" : "filtered_html",
          "safe_value" : "\u003Cp\u003E\u003Ca href=\u0022\/citations\u0022\u003ENew [message:field_boards] articles posted.\u003C\/a\u003E  [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
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
          "value" : "\u003Cp\u003ENew event \u003Ca href=\u0022\/event\/[message:field_event_id]\u0022\u003E[message:field_title]\u003C\/a\u003E posted. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E",
          "format" : "filtered_html",
          "safe_value" : "\u003Cp\u003ENew event \u003Ca href=\u0022\/event\/[message:field_event_id]\u0022\u003E[message:field_title]\u003C\/a\u003E posted. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
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
          "value" : "\u003Cp\u003E\u003Ca href=\u0022\/event\/[message:field_event_id]\u0022\u003E[message:field_title]\u003C\/a\u003E date or time updated. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E",
          "format" : "filtered_html",
          "safe_value" : "\u003Cp\u003E\u003Ca href=\u0022\/event\/[message:field_event_id]\u0022\u003E[message:field_title]\u003C\/a\u003E date or time updated. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
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
          "value" : "[message:field_title] cancelled.  [message:timestamp:custom:m\/d\/Y]",
          "format" : "plain_text",
          "safe_value" : "\u003Cp\u003E[message:field_title] cancelled.  [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
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
          "value" : "\u003Cp\u003EAgenda published for \u003Ca href=\u0022\/event\/[message:field_event_id]\u0022\u003E[message:field_title]\u003C\/a\u003E. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E",
          "format" : "filtered_html",
          "safe_value" : "\u003Cp\u003EAgenda published for \u003Ca href=\u0022\/event\/[message:field_event_id]\u0022\u003E[message:field_title]\u003C\/a\u003E. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
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
          "value" : "\u003Cp\u003E\u003Ca href=\u0022\/event\/[message:field_event_id]\u0022\u003E[message:field_title]\u003C\/a\u003E changed to [message:field_event_type]. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E",
          "format" : "filtered_html",
          "safe_value" : "\u003Cp\u003E\u003Ca href=\u0022\/event\/[message:field_event_id]\u0022\u003E[message:field_title]\u003C\/a\u003E changed to [message:field_event_type]. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
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
          "value" : "\u003Cp\u003E\u003Ca href=\u0022\/packet\/[message:field_packet_id]\u0022\u003E[message:field_title] literature posted.\u003C\/a\u003E [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E",
          "format" : "filtered_html",
          "safe_value" : "\u003Cp\u003E\u003Ca href=\u0022\/packet\/[message:field_packet_id]\u0022\u003E[message:field_title] literature posted.\u003C\/a\u003E [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
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
          "value" : "\u003Cp class=\u0022new_summary_message-[message:mid]\u0022\u003E[message:user:name] posted \u003Ca href=\u0022\/[message:field_summary_url]\u0022\u003E[message:field_title]\u003C\/a\u003E. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E",
          "format" : "full_html",
          "safe_value" : "\u003Cp class=\u0022new_summary_message-[message:mid]\u0022\u003E[message:user:name] posted \u003Ca href=\u0022\/[message:field_summary_url]\u0022\u003E[message:field_title]\u003C\/a\u003E. [message:timestamp:custom:m\/d\/Y]\u003C\/p\u003E\n"
        }
      ]
    }
  }'
);
