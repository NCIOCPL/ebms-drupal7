<?php
/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

dsm(get_defined_vars(), 'vars');
?>

<?php if(!$in_preview): ?>
<div id='left-nav'>
    <?php
    module_load_include('inc', 'ebms', 'calendar');
    print ebms_calendar_left_nav();
    ?>
</div>
<div id='calendar-enclosure' <?php if ($editor) print 'class="editable"'; ?>>

    <div id='event-pager'>
        <div id='nav-previous-event'>
            <?php
            if (isset($prevNode))
                print l('<', "node/$prevNode");
            ?>
        </div>
        <div id='nav-following-event'>
            <?php
            if (isset($nextNode))
                print l('>', "node/$nextNode");
            ?>
        </div>
    </div>

    <?php
    if ($editor) {
        print "<div class='edit-button-icon'><a href='$editNodePath'><img alt='Edit Event' src='$editIconPath'></a></div>";
    }
    ?> 
    <?php else: ?>
    <div class='preview-enclosure'>
    <?php endif; ?>
    <h2 class='indent trailing'> 
        <?php
        if ($cancelled)
            print "Cancelled: ";

        print $title;
        ?>
    </h2>
    <div class="content"<?php print $content_attributes; ?>>
        <div class='indent trailing subheader'> <?php print $eventDate; ?> </div>
        <div class='indent trailing'> <?php print $eventTime; ?> </div>

        <div class='indent subheader'>Event Type</div>
        <div class='indent trailing'> <?php print $eventType; ?> </div>

        <?php if ($boardName) : ?>
            <div class='indent subheader'>Boards</div>
            <div class='indent trailing'><?php print $boardName; ?></div>
            <?php
        endif;

        if ($individuals) :
            ?>
            <div class='indent subheader'>Individuals</div>
            <div class='indent trailing'> <?php print $individuals; ?> </div>
            <?php
        endif;

        if ($agenda) :
            ?>
            <div class='indent'><span class='subheader'>Agenda</span><i><?php print $agenda_status; ?></i></div>
            <div class='indent trailing'> <?php print $agenda; ?> </div>

            <?php
        endif;

        if ($eventNotes) :
            ?>
            <div class='indent subheader'>Notes</div>
            <div class='indent trailing'> <?php print $eventNotes; ?> </div>
        <?php endif; ?>

        <div class='indent trailing'> <?php print "<i>Submitted by $submitter on $submitted</i>"; ?> </div>

        <?php if (!empty($docLinks)) : ?>
            <div class='trailing'>
                <div class='indent subheader'>Event Documents</div>

                <?php
                foreach ($docLinks as $link)
                        print "<div class='indent'>$link</div>";
                ?>
            </div>
        <?php endif; ?>

        <div class='trailing subheader indent'>
            <?php
            print l('Add to Personal Calendar', "node/$nid/event.ics");
            ?>
        </div>
    </div>
</div>
