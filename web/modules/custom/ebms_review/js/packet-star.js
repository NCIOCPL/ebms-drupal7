/**
 * Toggle the packet's "starred" setting with an AJAX call.
 *
 * @param {int} packet_id
 * @param {int} flag
 */
function flip_packet_star(packet_id, flag) {
  var url = "/packets/packet/star/" + packet_id + "/" + flag;
  jQuery("#packet-star-" + packet_id).load(url);
}
