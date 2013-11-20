/**
 * $Id$
 *
 * OCEEBMS-81: open linked documents from calendar events to a new tab.
 */
CKEDITOR.on('dialogDefinition', function(e) {
    if (e.data.name == 'link') {
        e.data.definition.getContents('target')
            .get('linkTargetType')['default'] = '_blank';
    }
});
