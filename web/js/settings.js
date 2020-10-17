function showForm() {
    $('form').show();
    $('.loading').hide();
}

function showLoading() {
    $('form').hide();
    $('.loading').show();
}

function update(data) {
    $.ajax({
        url: "/api/set",
        dataType: "json",
        data: data,
        success: function (data) {
            $.each(data, function (key) {
                $('input[name=' + key + ']').val(data[key]);
            });

            showForm();
        }
    });
}

$(document).ready(function () {
    let form = $('form');
    update({});

    form.on('submit', function (e) {
        e.preventDefault();
        showLoading();

        let data = {};
        let form = $(this);
        form.find('input[name]').each(function () {
            let that = $(this);
            data[that.attr('name')] = that.val();
        });

        update(data);
    });
});
