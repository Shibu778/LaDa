# /* This code was taken from boost/preprocessor file and modified
#    for LaDa. Is it redistributed with the original license below.  */
#
# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
# /* Boost Software License - Version 1.0 - August 17th, 2003 
#
# Permission is hereby granted, free of charge, to any person or organization
# obtaining a copy of the software and accompanying documentation covered by
# this license (the "Software") to use, reproduce, display, distribute,
# execute, and transmit the Software, and to prepare derivative works of the
# Software, and to permit third-parties to whom the Software is furnished to
# do so, all subject to the following:
#
# The copyright notices in the Software and this entire statement, including
# the above license grant, this restriction and the following disclaimer,
# must be included in all copies of the Software, in whole or in part, and
# all derivative works of the Software, unless such copies or derivative
# works are solely in the form of machine-executable object code generated by
# a source language processor.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
# SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
# FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE. */
#
# ifndef LADA_CRYSTAL_PPSLOT_HPP
# define LADA_CRYSTAL_P_SLOT_HPP
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/slot/detail/def.hpp>
#
# /* BOOST_PP_ASSIGN_SLOT */
#
# define LADA_ASSIGN_SLOT(i) BOOST_PP_CAT(LADA_ASSIGN_SLOT_, i)
#
# define LADA_ASSIGN_SLOT_crystal <python/ppslot_crystal.hpp>
# define LADA_ASSIGN_SLOT_python <python/ppslot_python.hpp>
#
# /* BOOST_PP_SLOT */
#
# define LADA_SLOT(i) BOOST_PP_CAT(LADA_SLOT_, i)()
#
# endif
